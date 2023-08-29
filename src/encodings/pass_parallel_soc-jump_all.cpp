#include "Pass_parallel_soc-jump_all.hpp"

using namespace std;

int Pass_parallel_socjump_all::Solve(int ags)
{
	delta = 0;
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
	CNF = vector<vector<int> >();
	int last_lit = 0; // we can reuse the CNF, just have to delete extra clauses when searching for soc
	int last_clause = 0;

	// find makespan optimal solution
	cost_function = 1;
	while (true)
	{
		PrintSolveDetails();

		// create formula
		auto start = chrono::high_resolution_clock::now();
		last_lit = CreateMksFormula(CNF, time_left);
		last_clause = CNF.size();
		auto stop = chrono::high_resolution_clock::now();
		building_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= building_time/1000;
		if (time_left < 0)
			return 1;

		// solve formula
		nr_vars = last_lit;
		nr_clauses = last_clause;
		PrintCNF(CNF);
		start = chrono::high_resolution_clock::now();
		res = InvokeSolver(time_left + 1, false);
		stop = chrono::high_resolution_clock::now();
		solving_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= solving_time/1000;
		if (time_left < 0)
			return 1;

		if (res == 0) // ok
			break;
		if (res == -1) // something went horribly wrong with the solver!
			return 1;
		return 1;
		delta++; // no solution with given limits, increase delta
	}

	

	// optimize soc wtih the current formula using bin. search
	cost_function = 2;
	int delta_backup = delta;
	int delta_LB = delta;
	int delta_UB = agents * inst->GetMksLB(agents) - inst->GetSocLB(agents) - 1;
	int lowest_sat = delta_UB + 1;

	while (delta_LB <= delta_UB)
	{
		delta = (delta_LB + delta_UB) / 2; 
		PrintSolveDetails();

		// create formula
		auto start = chrono::high_resolution_clock::now();
		nr_vars = CreateDeltaFormula(CNF, last_lit, delta);
		nr_clauses = CNF.size();
		auto stop = chrono::high_resolution_clock::now();
		building_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= building_time/1000;
		if (time_left < 0)
			return 1;

		// solve formula
		PrintCNF(CNF);
		start = chrono::high_resolution_clock::now();
		res = InvokeSolver(time_left + 1, false);
		stop = chrono::high_resolution_clock::now();
		solving_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= solving_time/1000;
		if (time_left < 0)
			return 1;

		CNF.erase(CNF.begin() + last_clause, CNF.end());

		if (res == 0) // sat
		{
			lowest_sat = min(delta, lowest_sat);
			delta_UB = delta - 1;
		}
		if (res == 1) // unsat
			delta_LB = delta + 1;
		if (res == -1) // something went horribly wrong with the solver!
			return 1;
	}

	// find optimal soc with the given delta
	CleanUp();
	CNF = vector<vector<int> >();
	delta = lowest_sat;
	int cost = 0;
	int cost_LB = delta_backup;
	int cost_UB = delta - 1;

	// create common soc formula
	auto start = chrono::high_resolution_clock::now();
	last_lit = CreateSocFormula(CNF, last_lit);
	last_clause = CNF.size();
	auto stop = chrono::high_resolution_clock::now();
	building_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
	time_left -= building_time/1000;
	if (time_left < 0)
		return 1;
	
	while (cost_LB <= cost_UB)
	{
		cost = (cost_LB + cost_UB) / 2; 
		PrintSolveDetails();

		// create formula
		auto start = chrono::high_resolution_clock::now();
		nr_vars = CreateDeltaFormula(CNF, last_lit, cost);
		nr_clauses = CNF.size();
		auto stop = chrono::high_resolution_clock::now();
		building_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= building_time/1000;
		if (time_left < 0)
			return 1;

		// solve formula
		PrintCNF(CNF);
		start = chrono::high_resolution_clock::now();
		res = InvokeSolver(time_left + 1, false);
		stop = chrono::high_resolution_clock::now();
		solving_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= solving_time/1000;
		if (time_left < 0)
			return 1;

		CNF.erase(CNF.begin() + last_clause, CNF.end());

		if (res == 0) // sat
		{
			lowest_sat = min(cost, lowest_sat);
			cost_UB = cost - 1;
		}
		if (res == 1) // unsat
			cost_LB = cost + 1;
		if (res == -1) // something went horribly wrong with the solver!
			return 1;
	}

	// log results
	log->nr_vars = nr_vars;
	log->nr_clauses = nr_clauses;
	log->building_time = building_time;
	log->solving_time = solving_time;
	log->solution_mks = inst->GetMksLB(agents) + delta;
	log->solution_soc = inst->GetSocLB(agents) + lowest_sat;
	log->solver_calls = solver_calls;

	return 0;
}

int Pass_parallel_socjump_all::CreateMksFormula(vector<vector<int> >& CNF, int time_left)
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
	CreateConf_Swapping_Pass(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	// movement 
	CreateMove_NoDuplicates(CNF);
	CreateMove_EnterVertex_Pass(CNF);
	CreateMove_LeaveVertex_Pass(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	return lit;
}

int Pass_parallel_socjump_all::CreateSocFormula(vector<vector<int> >& CNF, int time_left)
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

	return lit;
}

int Pass_parallel_socjump_all::CreateDeltaFormula(std::vector<std::vector<int> >& CNF, int last_lit, int limit)
{
	vector<int> late_variables;

	for (int a = 0; a < agents; a++)
	{
		int goal_v = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];
		int at_var = at[a][goal_v].first_variable;
		int last_timestep = at[a][goal_v].last_timestep - at[a][goal_v].first_timestep;
		for (int d = 0; d < last_timestep; d++)
		{
			CNF.push_back(vector<int> {at_var + d, last_lit});	// if agent is not at goal, it is late
			if (d < last_timestep - 1)
				CNF.push_back(vector<int> {last_lit, -(last_lit + 1)});	// if agent is late at t, it is late at t+1
			late_variables.push_back(last_lit);
			last_lit++;
		}
	}
	// add constraint on sum of delays
	PB2CNF pb2cnf;
	vector<vector<int> > formula;
	last_lit = pb2cnf.encodeAtMostK(late_variables, limit, formula, last_lit) + 1;

	for (size_t i = 0; i < formula.size(); i++)
		CNF.push_back(formula[i]);

	return last_lit;
}


