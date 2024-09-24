#include "solver_common.hpp"

using namespace std;

int VarToID(int var, bool duplicate, int& freshID, unordered_map<int, int>& dict);

/** Constructor of _MAPFSAT_PassParallelMksAll.
*
* @param sol_name name of the encoding used in log. Defualt value is pass_parallel_mks_all.
*/
_MAPFSAT_MonosatPebbleMksAll::_MAPFSAT_MonosatPebbleMksAll(string sol_name)
{
	solver_name = sol_name;
	cost_function = 1; // 1 = mks, 2 = soc
	movement = 2; // 1 = parallel, 2 = pebble
	lazy_const = 1; // 1 = all at once, 2 = lazy
	solver_to_use = 2; // 1 = kissat, 2 = monosat
};

int _MAPFSAT_MonosatPebbleMksAll::CreateFormula(vector<vector<int> >& CNF, int time_left)
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
	CreatePossition_Start(CNF);
	CreatePossition_Goal(CNF);

	// conflicts
	CreateConf_Vertex(CNF);
	CreateConf_Pebble_Pass(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// not needed!!!
	CreateMove_NoDuplicates(CNF);

	// create graph
	//CreateMove_Graph_Monosat();

	// avoid locations - user has to make sure the avoid locations are pebble movement compatible
	CreateConst_Avoid(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// Deallocate memory
	//CleanUp(print_plan);

	return lit;
}
