#include "solver_common.hpp"

using namespace std;

/** Constructor of _MAPFSAT_SMT.
*
* Some combinations are not implemented yet. A standalone binary of Monosat solver is used, printing the formula into file is required!
*
* @param var variables to be use. Possible values 1 = at, 2 = pass, 3 = shift.
* @param cost optimized cost function. Possible values 1 = mks, 2 = soc.
* @param moves allowed movement. Possible values 1 = parallel, 2 = pebble.
* @param lazy eager or lazy solving of conflicts. Possible values 1 = eager, 2 = lazy.
* @param dupli allow of forbid duplicating agents. Possible values 1 = forbid, 2 = allow.
* @param solver SAT solver to be used. Possible values 1 = kissat, 2 = monosat.
* @param sol_name name of the encoding used in log. Defualt value is at_parallel_mks_all.
*/
_MAPFSAT_SMT::_MAPFSAT_SMT(int var, int cost, int moves, int lazy, int dupli, int solver, string sol_name)
{
	solver_name = sol_name;
	variables = var; 			// 1 = at, 			2 = pass, 		3 = shift
	cost_function = cost; 		// 1 = mks, 		2 = soc
	movement = moves; 			// 1 = parallel, 	2 = pebble
	lazy_const = lazy; 			// 1 = eager, 		2 = lazy
	duplicates = dupli; 		// 1 = forbid, 		2 = allow
	solver_to_use = solver; 	// 1 = kissat, 		2 = monosat

	assert(variables == 2 || variables == 3);
	assert(cost_function == 1 || cost_function == 2);
	assert(movement == 1);
	assert(lazy_const == 1);
	assert(duplicates == 1 || duplicates == 2);
	assert(solver_to_use == 2);
};

int _MAPFSAT_SAT::CreateFormula(int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;

	int lit = 1;

	auto start = chrono::high_resolution_clock::now();

	// create variables
	lit = CreateAt(lit, timesteps);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// start - goal possitions
	CreatePossition_Start();
	CreatePossition_Goal();

	// conflicts
	CreateConf_Vertex();
	CreateConf_Swapping_At();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	// movement 
	//CreateMove_NoDuplicates();
	CreateMove_NextVertex_At();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// avoid locations
	CreateConst_Avoid();
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;


	return lit;
}
