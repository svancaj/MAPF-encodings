#include "solver_common.hpp"

using namespace std;

/** Constructor of _MAPFSAT_PassParallelSocLazy.
*
* @param sol_name name of the encoding used in log. Defualt value is pass_parallel_soc_all.
*/
_MAPFSAT_PassParallelSocLazy::_MAPFSAT_PassParallelSocLazy(string sol_name)
{
	solver_name = sol_name;
	cost_function = 2; // 1 = mks, 2 = soc
	movement = 1; // 1 = parallel, 2 = pebble
	lazy_const = 2; // 1 = all at once, 2 = lazy
};

int _MAPFSAT_PassParallelSocLazy::CreateFormula(int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;

	int lit = 1;
	conflicts_present = false;

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
	CreateConf_Vertex_OnDemand();
	CreateConf_Swapping_Pass_OnDemand();
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
	CleanUp(true); // always keep at variables in lazy encoding to figure out the conflicts

	return lit;
}