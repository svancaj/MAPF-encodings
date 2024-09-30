#include "solver_common.hpp"

using namespace std;

/** Constructor of _MAPFSAT_ShiftParallelMksAll.
*
* @param sol_name name of the encoding used in log. Defualt value is shift_parallel_mks_all.
*/
_MAPFSAT_ShiftParallelMksAll::_MAPFSAT_ShiftParallelMksAll(string sol_name)
{
	solver_name = sol_name;
	cost_function = 1; // 1 = mks, 2 = soc
	movement = 1; // 1 = parallel, 2 = pebble
	lazy_const = 1; // 1 = all at once, 2 = lazy
};

int _MAPFSAT_ShiftParallelMksAll::CreateFormula(int time_left)
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
	CreateConf_Swapping_Shift();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	// movement 
	CreateMove_NoDuplicates();
	CreateMove_NextVertex_At();
	CreateMove_ExactlyOne_Shift();
	CreateMove_NextVertex_Shift();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// avoid locations
	CreateConst_Avoid();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// Deallocate memory
	CleanUp(print_plan);

	return lit;
}
