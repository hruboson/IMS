#include <iostream>
#include <stdlib.h>
#include <simlib.h>

//--------------------------------------------------------------------------//
// Simulation parameters 													//
//--------------------------------------------------------------------------//

#define S 100000 // number of seeds (initial)
#define W 2 // number of workers
#define P 2 // number of working stations
#define B 1 // number of packaging stations

#define X 40 // number of planting plates
#define Y 30 // planting plate capacity
#define I 200 // number of flowerpots
#define J 30 // flowerpot capacity

#define SHIFT_T 8*60 // one shift length (working time)
#define DAYS 365
#define TIMESPAN DAYS*24*60 // duration of simulation in minutes

//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// Facts																	//
//--------------------------------------------------------------------------//

#define S_T Y/3 // time to plant seeds in one planting plate TODO change
#define RP_T J/3 // time to replant plants to one flowerpot TODO change

#define OUT_T (24*60)-SHIFT_T // rest of the day

#define CONTROL_CHECK_T 4*60 // control panel timer
#define CONTROL_T 5 // time to check and set control panel
#define PACKING_T 0.25 // time to pack one unit (~15 seconds)

//--------------------------------------------------------------------------//

Queue workers("Pracovníci");
Queue workersOut("Pracovníci mimo");

Store workingStation("Pracovní stanice", P);
Store packagingStation("Balicí stanice", B);
Facility controlPanelFac("Ovládací panel");

Queue replantingQ("Sazenice na přesazení");
Queue readyForHarvest("Připraveno na sklizení");
Store seedingPlates("Sadbovací pláty", X);
Store flowerpots("Květináče", I);

bool shift_end = false;
bool control_panel_ready = false;
unsigned seeds = S;

Stat grownSeedsStat("Počet vyrostlých sazenic");
unsigned grownSeeds = 0;

Stat grownPlantsStat("Počet vyrostlých salátů");
unsigned grownPlants = 0;

Stat harvestedInTime("Počet sklizených salátů");
Histogram harvestTime("Počet sklizených salátu v jednotlivých dnech", 0, 1440, DAYS);
Histogram replantingTime("Počet přesazených salátů v jednotlivých dnech", 0, 1440, DAYS);
Histogram seedingTime("Počet zasazených semínek v jednotlivých dnech", 0, 1440, DAYS);

class PlantGrowing : public Process {
	void Behavior(){
		replantingTime(Time);
		Wait(Normal(34560, 2000)); // growing phase
		grownPlants += 1;
		grownPlantsStat(1);
		double time = Time;
		Into(readyForHarvest); // wait for harvest
		Passivate();

		// harvesting started
		if(grownPlants >= J){
			Leave(flowerpots, 1); // put flowerpot back
			grownPlants -= J;
		}

		if(Random() <= 0.03){ // 3% of harvested plants are bad (disease, rotten, etc.)
			return;
		}

		harvestTime(Time);
		harvestedInTime(Time-time);
	}
};

class SeedGrowing : public Process {
	void Behavior() {
		seedingTime(Time);
		Wait(Normal(8640, 500)); // growing phase
		grownSeeds += 1;
		grownSeedsStat(1);
		Into(replantingQ); // wait for replanting
		Passivate();
		
		// replanting started
		if(grownSeeds >= Y){
			Leave(seedingPlates, 1); // put seeding plate back
			grownSeeds -= Y;
		}

		// start plant growing phase
		(new PlantGrowing)->Activate();
    }
};

class ControlPanelTimer : public Process {
	void Behavior(){
		Wait(CONTROL_CHECK_T);
		control_panel_ready = true;

		if(workers.Length() > 0){
			(workers.GetFirst())->Activate();
		}
	}
};

class ShiftTimer : public Process {
	void Behavior(){
		while(true){
			Wait(SHIFT_T);
			shift_end = true;

			for(auto i = workers.begin(); i != workers.end(); ++i){
				Entity *proc = workers.Get(i);
				proc->Activate();
			}

			Wait(OUT_T);
			shift_end = false;

			for(auto i = workersOut.begin(); i != workersOut.end(); ++i){
				Entity *proc = workersOut.Get(i);
				proc->Activate();
			}
		}
	}
};



class Worker : public Process {
	void Behavior(){
		again:

		// from most prioritized: shift ended -> replanting seeds -> packaging/harvesting grown plant -> checking control panel -> planting seeds
		if(!shift_end){
			if(!replanting()){
				if(!packaging()){
					if(!controlPanel()){
						if(!seeding()){
						}
					}
				}
			}
		}else{
			Into(workersOut);
			Passivate();

			goto again;
		}

		Into(workers);
		Passivate();

		goto again;
	}

	bool controlPanel(){
		if(!controlPanelFac.Busy() && control_panel_ready){ // control panel needs checking
			Seize(controlPanelFac);
			Wait(CONTROL_T);
			Release(controlPanelFac); // checked
			
			control_panel_ready = false;
			(new ControlPanelTimer)->Activate(); // control panel doesn't need checking for the next two hours

			if(shift_end){
				Into(workersOut);
				Passivate();
			}

			if(workers.Length() > 0){
				(workers.GetFirst())->Activate();
			}

			return true;
		}

		return false;
	}

	bool seeding(){
		if(seeds >= Y){ // there are seeds to be planted
			if(seedingPlates.Free() > 0 && workingStation.Free() > 0){ // we have seeding plates and working station
				Enter(seedingPlates, 1);
				Enter(workingStation, 1);
				seeds -= Y; // subtrack from the overall number of seeds
				Wait(S_T); // planting takes some time
				Leave(workingStation, 1); // leave workstation (seeding plate is freed when seed grows)

				for(int i = 0; i < Y; i++){ // start growing process for each seed on seeding plate
					(new SeedGrowing)->Activate();
				}

				if(shift_end){
					Into(workersOut);
					Passivate();
				}

				if(workers.Length() > 0){
					(workers.GetFirst())->Activate();
				}

				return true;
			}
		}

		return false;
	}

	bool replanting(){
		if(flowerpots.Free() > 0 && replantingQ.Length() > 1 && workingStation.Free() > 0){ // we have flowerpots, workstation and plants to replant
			Enter(flowerpots, 1);
			Enter(workingStation, 1);

			Wait(RP_T);
			for(int i = 0; i < J; i++){ // start plant growing process for each plant in flowerpot
				if(replantingQ.Empty()){
					break;
				}
				(replantingQ.GetFirst())->Activate();
			}

			Leave(workingStation, 1); // leave workstation (flowerpot is freed when plant completely grows)

			if(shift_end){
				Into(workersOut);
				Passivate();

				return false;
			}

			if(workers.Length() > 0){
				(workers.GetFirst())->Activate();
			}

			return true;
		}

		return false;
	}

	bool packaging(){
		if(readyForHarvest.Length() > 0 && packagingStation.Free() > 0){ // we have some plants to harvest and packaging station is unoccupied
			Enter(packagingStation, 1);
			Wait(PACKING_T);
			if(readyForHarvest.Length() > 0){
				(readyForHarvest.GetFirst())->Activate(); // start packaging process for plant
			}
			Leave(packagingStation, 1);

			if(shift_end){
				Into(workersOut);
				Passivate();
			if(workers.Length() > 0){
				(workers.GetFirst())->Activate();
			}
				return false;
			}

			if(workers.Length() > 0){
				(workers.GetFirst())->Activate();
			}
		}

		return false;
	}
};

int main(){
	SetOutput("sim.dat");
	
    Init(0, TIMESPAN);

	for(int i = 0; i < W; i++){
		(new Worker)->Activate();
	}

	(new ControlPanelTimer)->Activate();
	(new ShiftTimer)->Activate();

    Run();

	/*controlPanelFac.Output();
	workers.Output();
	workersOut.Output();
	grownSeedsStat.Output();
	grownPlantsStat.Output();
	seedingPlates.Output();
	flowerpots.Output();
	replantingQ.Output();
	readyForHarvest.Output();
	harvestedPlants.Output();*/

	workers.Output();
	seedingTime.Output();
	replantingTime.Output();
	harvestTime.Output();
	harvestedInTime.Output();

    return EXIT_SUCCESS;
}
