#include "solver_common.hpp"

using namespace std;

/** Constructor of _MAPFSAT_PassParallelSocAll.
*
* @param sol_name name of the encoding used in log. Defualt value is pass_parallel_soc_all.
*/
_MAPFSAT_PassParallelSocAll::_MAPFSAT_PassParallelSocAll(string sol_name)
{
	solver_name = sol_name;
	cost_function = 2; // 1 = mks, 2 = soc
	movement = 1; // 1 = parallel, 2 = pebble
	lazy_const = 1; // 1 = all at once, 2 = lazy
};

int _MAPFSAT_PassParallelSocAll::CreateFormula(int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;

	int lit = 1;

	auto start = chrono::high_resolution_clock::now();

	// create variables
	lit = CreateAt(lit, timesteps);
	lit = CreatePass(lit, timesteps);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// start - goal possitions
	CreatePossition_Start();
	CreatePossition_Goal();
	CreatePossition_NoneAtGoal();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// conflicts
	CreateConf_Vertex();
	CreateConf_Swapping_Pass();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	// movement 
	CreateMove_NoDuplicates();
	CreateMove_EnterVertex_Pass();
	CreateMove_LeaveVertex_Pass();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// soc limit
	if (delta > 0)
		lit = CreateConst_LimitSoc(lit);

	// avoid locations
	CreateConst_Avoid();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// Deallocate memory
	CleanUp(print_plan);

	return lit;
}
