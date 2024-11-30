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
#define TIMESPAN 365*24*60 // duration of simulation in minutes

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
Queue seeds("Semínka");

Facility control_panel("Ovládací panel");

class Worker : public Process {
	void Behavior(){
		again:

		checkControlPanel();

		Into(workers);
		Passivate();
		std::cout << "Activated" << std::endl;

		goto again;
	}

	void checkControlPanel(){
		if(!control_panel.Busy()){
			Seize(control_panel);
			Wait(CONTROL_T);
			Release(control_panel);


			if(workers.Length() > 0){
				std::cout << "Activate next" << std::endl;
				(workers.GetFirst())->Activate();
			}

			return;
		}
	}
};

class Seeds : public Process{
	void Behavior() {
    }
};

int main(){
	
    Init(0, TIMESPAN);

	for(int i = 0; i < W; i++){
		(new Worker)->Activate();
	}

    Run();
	
	control_panel.Output();
	workers.Output();

    return EXIT_SUCCESS;
}
