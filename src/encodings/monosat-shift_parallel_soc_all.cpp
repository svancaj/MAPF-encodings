#include "solver_common.hpp"

using namespace std;

/** Constructor of _MAPFSAT_MonosatShiftParallelSocAll.
*
* @param sol_name name of the encoding used in log. Defualt value is pass_parallel_mks_all.
*/
_MAPFSAT_MonosatShiftParallelSocAll::_MAPFSAT_MonosatShiftParallelSocAll(string sol_name)
{
	solver_name = sol_name;
	cost_function = 2; // 1 = mks, 2 = soc
	movement = 1; // 1 = parallel, 2 = pebble
	lazy_const = 1; // 1 = all at once, 2 = lazy
	solver_to_use = 2; // 1 = kissat, 2 = monosat
};

int _MAPFSAT_MonosatShiftParallelSocAll::CreateFormula(int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;

	int lit = 1;

	auto start = chrono::high_resolution_clock::now();

	// create variables
	lit = CreateShift(lit, timesteps);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// conflicts
	// no need to explicitly forbid vertex conflicts, it is forbidden by "at most 1 shift"
	CreateConf_Swapping_Shift();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// limit number of shifts
	CreateMove_ExactlyOne_Shift();
	CreateMove_ExactlyOneIncoming_Shift();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// soc limit
	if (delta > 0)
		lit = CreateConst_LimitSoc_AllAt(lit);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// create movement graph
	lit = CreateMove_Graph_MonosatShift(lit);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// avoid locations		// not supported for shift without At
	//CreateConst_Avoid();
	//if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
	//	return -1;

	// Deallocate memory
	CleanUp(print_plan);

	return lit;
}
