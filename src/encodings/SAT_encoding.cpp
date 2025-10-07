#include "solver_common.hpp"

using namespace std;

/** Constructor of _MAPFSAT_SAT.
*
* @param var variables to be use. Possible values 1 = at, 2 = pass, 3 = shift.
* @param cost optimized cost function. Possible values 1 = mks, 2 = soc.
* @param moves allowed movement. Possible values 1 = parallel, 2 = pebble.
* @param lazy eager or lazy solving of conflicts. Possible values 1 = eager, 2 = lazy.
* @param dupli allow of forbid duplicating agents. Possible values 1 = forbid, 2 = allow.
* @param solver SAT solver to be used. Possible values 1 = kissat, 2 = monosat.
* @param sol_name name of the encoding used in log. Defualt value is at_parallel_mks_all.
*/
_MAPFSAT_SAT::_MAPFSAT_SAT(int var, int cost, int moves, int lazy, int dupli, int solver, string sol_name)
{
	solver_name = sol_name;
	variables = var; 			// 1 = at, 			2 = pass, 		3 = shift
	cost_function = cost; 		// 1 = mks, 		2 = soc
	movement = moves; 			// 1 = parallel, 	2 = pebble
	lazy_const = lazy; 			// 1 = eager, 		2 = lazy
	duplicates = dupli; 		// 1 = forbid, 		2 = allow
	solver_to_use = solver; 	// 1 = kissat, 		2 = monosat

	assert(variables == 1 || variables == 2 || variables == 3);
	assert(cost_function == 1 || cost_function == 2);
	assert(movement == 1 || movement == 2);
	assert(lazy_const == 1 || lazy_const == 2);
	assert(duplicates == 1 || duplicates == 2);
	assert(solver_to_use == 1);
};

int _MAPFSAT_SAT::CreateFormula(int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;
	int lit = 1;
	auto start = chrono::high_resolution_clock::now();

	/********************/
	/* create variables */
	/********************/
	lit = CreateAt(lit, timesteps);
	if (variables == 2)
		lit = CreatePass(lit, timesteps);
	if (variables == 3)
		lit = CreateShift(lit, timesteps);

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	/***************************/
	/* start - goal possitions */
	/***************************/
	CreatePossition_Start();
	CreatePossition_Goal();
	if (cost_function == 2)
			CreatePossition_NoneAtGoal();

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	/*************/
	/* conflicts */
	/*************/
	if (lazy_const == 1)	// eager
	{
		CreateConf_Vertex();
		if (movement == 1)	// parallel
		{
			if (variables == 1)
				CreateConf_Swapping_At();
			if (variables == 2)
				CreateConf_Swapping_Pass();
			if (variables == 3)
				CreateConf_Swapping_Shift();
		}
		if (movement == 2)	// pebble
		{
			if (variables == 1)
				CreateConf_Pebble_At();
			if (variables == 2)
				CreateConf_Pebble_Pass();
			if (variables == 3)
				CreateConf_Pebble_Shift();
		}
	}
	if (lazy_const == 2)	// lazy
	{
		CreateConf_Vertex_OnDemand();
		if (movement == 1)	// parallel
		{
			if (variables == 1)
				CreateConf_Swapping_At_OnDemand();
			if (variables == 2)
				CreateConf_Swapping_Pass_OnDemand();
			if (variables == 3)
				CreateConf_Swapping_Shift_OnDemand();
		}
		if (movement == 2)	// pebble
		{
			if (variables == 1)
				CreateConf_Pebble_At_OnDemand();
			if (variables == 2)
				CreateConf_Pebble_Pass_OnDemand();
			if (variables == 3)
				CreateConf_Pebble_Shift_OnDemand();
		}
	}

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	/************/
	/* movement */
	/************/
	if (duplicates == 1)
		CreateMove_NoDuplicates();

	if (variables == 1)		// at
		CreateMove_NextVertex_At();
	if (variables == 2)		// pass
	{
		CreateMove_EnterVertex_Pass();
		CreateMove_NextEdge_Pass();
	}
	if (variables == 3)		// shift
	{
		CreateMove_NextVertex_At();
		CreateMove_ExactlyOne_Shift();
		CreateMove_NextVertex_Shift();
	}

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;


	/**************/
	/* limit cost */
	/**************/
	if (cost_function == 2 && delta > 0)
	{
		if (duplicates == 1)
			lit = CreateConst_LimitSoc(lit);
		if (duplicates == 2)
			lit = CreateConst_LimitSoc_AllAt(lit);
	}

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	/*******************/
	/* avoid locations */
	/*******************/
	if (use_avoid)
		CreateConst_Avoid();

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	return lit;
}
