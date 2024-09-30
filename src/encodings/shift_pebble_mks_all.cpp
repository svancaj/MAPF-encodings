#include "solver_common.hpp"

using namespace std;

/** Constructor of _MAPFSAT_ShiftPebbleMksAll.
*
* @param sol_name name of the encoding used in log. Defualt value is shift_pebble_mks_all.
*/
_MAPFSAT_ShiftPebbleMksAll::_MAPFSAT_ShiftPebbleMksAll(string sol_name)
{
	solver_name = sol_name;
	cost_function = 1; // 1 = mks, 2 = soc
	movement = 2; // 1 = parallel, 2 = pebble
	lazy_const = 1; // 1 = all at once, 2 = lazy
};

int _MAPFSAT_ShiftPebbleMksAll::CreateFormula(int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;

	int lit = 1;

	auto start = chrono::high_resolution_clock::now();

	// create variables
	lit = CreateAt(lit, timesteps);
	lit = CreateShift(lit, timesteps);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// start - goal possitions
	CreatePossition_Start();
	CreatePossition_Goal();

	// conflicts
	CreateConf_Vertex();
	//CreateConf_Swapping_Shift();
	CreateConf_Pebble_Shift();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	// movement 
	CreateMove_NoDuplicates();
	CreateMove_NextVertex_At();
	CreateMove_ExactlyOne_Shift();
	CreateMove_NextVertex_Shift();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// avoid locations - user has to make sure the avoid locations are pebble movement compatible
	CreateConst_Avoid();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// Deallocate memory
	CleanUp(print_plan);

	return lit;
}
