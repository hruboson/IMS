#include <iostream>
#include <stdlib.h>
#include <simlib.h>

//--------------------------------------------------------------------------//
// Simulation parameters 													//
//--------------------------------------------------------------------------//

#define S 1000 // number of seeds
#define W 4 // number of workers
#define P 2 // number of working stations
#define B 1 // number of packaging stations

#define X 20 // number of planting plates
#define Y 40 // planting plate capacity
#define I 10 // number of flowerpots
#define J 40 // flowerpot capacity

#define SHIFT_T 8*60 // one shift time (working time)
#define TIMESPAN 3*24*60 // duration of simulation in minutes

//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// Facts																	//
//--------------------------------------------------------------------------//

#define P_T X/3 // time to plant seeds in one planting plate TODO change
#define RP_T J/3 // time to replant plants to flowerpots TODO change

#define OUT_T (24*60)-SHIFT_T // rest of the day

#define CONTROL_CHECK_T 2*60 // control panel timer
#define PACKING_T 5 // time to pack one unit
#define CONTROL_T 5 // time to check and set control panel

//--------------------------------------------------------------------------//

Queue workers("Pracovníci");
Queue workersOut("Pracovníci mimo");
Queue seeds("Semínka");

Facility control_panel("Ovládací panel");

/*Store growingSeeds("Zasazené semínka", X*Y);
Store growingPlants("Rostoucí rostliny", I*J);

Store plantingSeeds("Sázení", X);
Store replantingPlants("Přesazování", I);*/

bool shift_end = false;
bool control_panel_ready = false;

class SeedGrowing : public Process {
	void Behavior() {
		/*Enter(growingSeeds);
		Wait()*/
    }
};

class PlantGrowing : public Process {
	void Behavior(){

	}
};

class ControlPanelTimer : public Process {
	void Behavior(){
		Wait(CONTROL_CHECK_T);
		control_panel_ready = true;

		if(workers.Length() > 0){
			(workers.GetFirst())->Activate();
		}
		std::cout << "CCT ready at " << Time << std::endl;
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
			std::cout << "Shift end at " << Time << std::endl;

			Wait(OUT_T);
			shift_end = false;

			for(auto i = workersOut.begin(); i != workersOut.end(); ++i){
				Entity *proc = workersOut.Get(i);
				proc->Activate();
			}
			std::cout << "Shift start at " << Time << std::endl;
		}
	}
};



class Worker : public Process {
	void Behavior(){
		again:

		if(!shift_end){
			if(!replanting()){
				if(!packaging()){
					if(!seeding()){
						if(!controlPanel()){
							std::cout << "Nothing: " << this->id() << std::endl;
						}
					}
				}
			}
		}else{
			std::cout << "Going out imm: " << this->id() << std::endl;
			Into(workersOut);
			Passivate();

			goto again;
		}

		Into(workers);
		Passivate();

		std::cout << "Checking work again: " << this->id() << std::endl;

		goto again;
	}

	bool controlPanel(){
		if(!control_panel.Busy() && control_panel_ready){
			Seize(control_panel);
			Wait(CONTROL_T);
			Release(control_panel);
			
			control_panel_ready = false;
			(new ControlPanelTimer)->Activate();

			if(shift_end){
				std::cout << "Going out control: " << this->id() << std::endl;
				Into(workersOut);
				Passivate();

				return false;
			}

			std::cout << "Control panel: " << this->id() << std::endl;

			if(workers.Length() > 0){
				(workers.GetFirst())->Activate();
			}

			return true;
		}

		return false;
	}

	bool seeding(){
		if(shift_end){
			std::cout << "Going out seeding: " << this->id() << std::endl;
			Into(workersOut);
			Passivate();

			return false;
		}
		return false;
	}

	bool replanting(){
		if(shift_end){
			std::cout << "Going out replanting: " << this->id() << std::endl;
			Into(workersOut);
			Passivate();

			return false;
		}
		return false;
	}

	bool packaging(){
		if(shift_end){
			std::cout << "Going out packaging: " << this->id() << std::endl;
			Into(workersOut);
			Passivate();

			return false;
		}
		return false;
	}
};

int main(){
	//SetOutput("sim.dat")
	
    Init(0, TIMESPAN);

	for(int i = 0; i < W; i++){
		(new Worker)->Activate();
	}

	(new ControlPanelTimer)->Activate();
	(new ShiftTimer)->Activate();

    Run();
	
	control_panel.Output();
	workers.Output();
	SIMLIB_statistics.Output();

    return EXIT_SUCCESS;
}
