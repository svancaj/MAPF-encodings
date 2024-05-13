#include "solver_common.hpp"

using namespace std;

/** Constructor of _MAPFSAT_AtPebbleMksAll.
*
* @param sol_name name of the encoding used in log. Defualt value is at_pebble_mks_all.
*/
_MAPFSAT_AtPebbleMksAll::_MAPFSAT_AtPebbleMksAll(string sol_name)
{
	solver_name = sol_name;
	cost_function = 1; // 1 = mks, 2 = soc
	movement = 2; // 1 = parallel, 2 = pebble
	lazy_const = 1; // 1 = all at once, 2 = lazy
};

int _MAPFSAT_AtPebbleMksAll::CreateFormula(vector<vector<int> >& CNF, int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;

	int lit = 1;

	auto start = chrono::high_resolution_clock::now();

	// create variables
	lit = CreateAt(lit, timesteps);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// start - goal possitions
	CreatePossition_Start(CNF);
	CreatePossition_Goal(CNF);

	// conflicts
	CreateConf_Vertex(CNF);
	CreateConf_Pebble_At(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	// movement 
	CreateMove_NoDuplicates(CNF);
	CreateMove_NextVertex_At(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// avoid locations - user has to make sure the avoid locations are pebble movement compatible
	CreateConst_Avoid(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// Deallocate memory
	CleanUp(print_plan);

	return lit;
}
