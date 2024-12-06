#include "solver_common.hpp"

using namespace std;

/** Constructor of _MAPFSAT_AtPebbleMksLazy.
*
* @param sol_name name of the encoding used in log. Defualt value is at_pebble_mks_all.
*/
_MAPFSAT_AtPebbleMksLazy::_MAPFSAT_AtPebbleMksLazy(string sol_name)
{
	solver_name = sol_name;
	cost_function = 1; // 1 = mks, 2 = soc
	movement = 2; // 1 = parallel, 2 = pebble
	lazy_const = 2; // 1 = all at once, 2 = lazy
};

int _MAPFSAT_AtPebbleMksLazy::CreateFormula(int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;

	int lit = 1;
	conflicts_present = false;

	auto start = chrono::high_resolution_clock::now();

	// create variables
	lit = CreateAt(lit, timesteps);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// start - goal possitions
	CreatePossition_Start();
	CreatePossition_Goal();

	// conflicts
	CreateConf_Vertex_OnDemand();
	CreateConf_Pebble_At_OnDemand();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	// movement 
	CreateMove_NoDuplicates();
	CreateMove_NextVertex_At();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// avoid locations - user has to make sure the avoid locations are pebble movement compatible
	CreateConst_Avoid();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// Deallocate memory
	CleanUp(true); // always keep at variables in lazy encoding to figure out the conflicts

	return lit;
}
