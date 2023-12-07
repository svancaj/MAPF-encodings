#include "solver_common.hpp"

using namespace std;

int Pass_parallel_soc_all::Solve(int ags, int input_delta, bool oneshot)
{
	delta = input_delta;
	int time_left = timeout; // in s
	long long building_time = 0;
	long long solving_time = 0;
	solver_calls = 0;
	int res = 1; // 0 = ok, 1 = timeout, -1 no solution

	at = NULL;
	pass = NULL;
	shift = NULL;
	agents = ags;
	vertices = inst->number_of_vertices;

	while (true)
	{
		PrintSolveDetails();

		// create formula
		auto start = chrono::high_resolution_clock::now();
		CNF = vector<vector<int> >();
		nr_vars = CreateFormula(CNF, time_left);
		nr_clauses = CNF.size();
		auto stop = chrono::high_resolution_clock::now();
		building_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= building_time/1000;
		if (time_left < 0)
			return 1;

		// solve formula
		start = chrono::high_resolution_clock::now();
		res = InvokeSolver(CNF, time_left + 1, print_plan);
		stop = chrono::high_resolution_clock::now();
		solving_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= solving_time/1000;
		if (time_left < 0)
			return 1;

		if (res == 0) // ok
		{
			log->nr_vars = nr_vars;
			log->nr_clauses = nr_clauses;
			log->building_time = building_time;
			log->solving_time = solving_time;
			log->solution_mks = inst->GetMksLB(agents) + delta;
			log->solution_soc = inst->GetSocLB(agents) + delta;
			log->solver_calls = solver_calls;
			break;
		}
		if (res == -1) // something went horribly wrong with the solver!
			return 1;

		delta++; // no solution with given limits, increase delta
		if (oneshot)	// no solution with the given delta, do not optimize, return no sol
			return -1;
	}

	return 0;
}

int Pass_parallel_soc_all::CreateFormula(vector<vector<int> >& CNF, int time_left)
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
	CreatePossition_NoneAtGoal(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// conflicts
	CreateConf_Vertex(CNF);
	CreateConf_Swapping_Pass(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	// movement 
	CreateMove_NoDuplicates(CNF);
	CreateMove_EnterVertex_Pass(CNF);
	CreateMove_LeaveVertex_Pass(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// soc limit
	if (delta > 0)
		lit = CreateConst_LimitSoc(CNF, lit);

	// Deallocate memory
	CleanUp(print_plan);

	return lit;
}
