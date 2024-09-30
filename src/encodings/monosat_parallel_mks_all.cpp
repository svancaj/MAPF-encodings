#include "solver_common.hpp"

using namespace std;

int VarToID(int var, bool duplicate, int& freshID, unordered_map<int, int>& dict);

/** Constructor of _MAPFSAT_PassParallelMksAll.
*
* @param sol_name name of the encoding used in log. Defualt value is pass_parallel_mks_all.
*/
_MAPFSAT_MonosatParallelMksAll::_MAPFSAT_MonosatParallelMksAll(string sol_name)
{
	solver_name = sol_name;
	cost_function = 1; // 1 = mks, 2 = soc
	movement = 1; // 1 = parallel, 2 = pebble
	lazy_const = 1; // 1 = all at once, 2 = lazy
	solver_to_use = 2; // 1 = kissat, 2 = monosat
};

int _MAPFSAT_MonosatParallelMksAll::CreateFormula(int time_left)
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

	// conflicts
	CreateConf_Vertex();
	CreateConf_Swapping_Pass();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// not needed!!!
	CreateMove_NoDuplicates();

	// create graph
	//CreateMove_Graph_Monosat();

	// avoid locations
	CreateConst_Avoid();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// Deallocate memory
	//CleanUp(print_plan);

	return lit;
}
