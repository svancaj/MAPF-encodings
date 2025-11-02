#include "solver_common.hpp"

/*
#pragma GCC diagnostic push // external libs have some warnings, lets ignore them
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Woverflow"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "../externals/pblib/pb2cnf.h" // https://github.com/master-keying/pblib
#include "../externals/monosat/api/Monosat.h" //https://github.com/sambayless/monosat
#pragma GCC diagnostic pop	// do not ignore warnings in our code!
*/

using namespace std;

/****************************/
// MARK: constructor
/****************************/

/** Constructor of _MAPFSAT_SMT.
*
* Some combinations are not implemented yet. A standalone binary of Monosat solver is used, printing the formula into file is required!
*
* @param var variables to be use. Possible values 1 = at, 2 = pass, 3 = shift.
* @param cost optimized cost function. Possible values 1 = mks, 2 = soc.
* @param moves allowed movement. Possible values 1 = parallel, 2 = pebble.
* @param lazy eager or lazy solving of conflicts. Possible values 1 = eager, 2 = lazy.
* @param dupli allow of forbid duplicating agents. Possible values 1 = forbid, 2 = allow.
* @param solver SAT solver to be used. Possible values 1 = CaDiCaL, 2 = monosat.
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
	solver_to_use = solver; 	// 1 = CaDiCaL, 		2 = monosat

	assert(variables == 2 || variables == 3);
	assert(cost_function == 1 || cost_function == 2);
	assert(movement == 1);
	assert(lazy_const == 1);
	assert(duplicates == 1 || duplicates == 2);
	assert(solver_to_use == 2);
};

/****************************/
// MARK: formula
/****************************/

int _MAPFSAT_SMT::CreateFormula(int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;

	int lit = 1;

	auto start = chrono::high_resolution_clock::now();

	/*******************************/
	/* create variables and graphs */
	/*******************************/
	if (variables == 2)
	{
		lit = CreateAt(lit, timesteps);
		lit = CreatePass(lit, timesteps);
		lit = CreateMove_Graph_MonosatPass(lit);
	}
	if (variables == 3)
	{
		lit = CreateShift(lit, timesteps);
		lit = CreateMove_Graph_MonosatShift(lit);
	}

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	/***************************/
	/* start - goal possitions */
	/***************************/
	if (cost_function == 2)
	{
		if (variables == 2)
			CreatePossition_NoneAtGoal();
		if (variables == 3)
			CreatePossition_NoneAtGoal_Shift();
	}

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	/*************/
	/* conflicts */
	/*************/
	if (variables == 2)
	{
		CreateConf_Vertex();
		CreateConf_Swapping_Pass();
	}
	if (variables == 3)
	{
		// no need to explicitly forbid vertex conflicts, it is forbidden by "at most 1 shift"
		CreateConf_Swapping_Shift();
	}

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	/************/
	/* movement */
	/************/
	if (variables == 2)
	{
		CreateConf_Vertex();
		CreateConf_Swapping_Pass();
		if (duplicates == 1)
			CreateMove_NoDuplicates();
	}
	if (variables == 3)
	{
		CreateMove_ExactlyOne_Shift();
		CreateMove_ExactlyOneIncoming_Shift();
		// forbidding duplicates for shift is much harder ...
	}

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	/**************/
	/* limit cost */
	/**************/
	if (cost_function == 2 && delta > 0)
	{
		if (variables == 2)
		{
			if (duplicates == 1)
				lit = CreateConst_LimitSoc(lit);
			if (duplicates == 2)
				lit = CreateConst_LimitSoc_AllAt(lit);
		}
		if (variables == 3)
			lit = CreateConst_LimitSoc_Shift(lit);
	}

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	
	/*******************/
	/* avoid locations */
	/*******************/
	if (variables == 2 && use_avoid)
		CreateConst_Avoid();

	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	return lit;
}

/****************************/
// MARK: graph generators
/****************************/

int _MAPFSAT_SMT::CreateMove_Graph_MonosatPass(int lit)
{
	for (int a = 0; a < agents; a++)
	{
		// GraphTheorySolver_long g_theory = newGraph((SolverPtr)SAT_solver);

		if (cnf_file.compare("") != 0)
			cnf_printable << "digraph int 0 0 " << a << "\n";
		
		int vertex_id = 0;
		unordered_map<int, int> dict;

		// turn vertices into edges
		for (int v = 0; v < vertices; v++)
		{
			if (at[a][v].first_variable == 0)
				continue;

			int star_t = at[a][v].first_timestep;
			int end_t = at[a][v].last_timestep + 1;

			for (int t = star_t; t < end_t; t++)
			{
				int at_var = at[a][v].first_variable + (t - at[a][v].first_timestep);

				int v1 = VarToID(at_var, false, vertex_id, dict);
				int v2 = VarToID(at_var, true, vertex_id, dict);

				//g_theory->newEdge(v1, v2, at_var);
				if (cnf_file.compare("") != 0)
					cnf_printable << "edge " << a << " " << v1 << " " << v2 << " " << at_var << "\n";
			}
		}

		// connect vertices based on pass varaibles
		for (int v = 0; v < vertices; v++)
		{
			if (at[a][v].first_variable == 0)
				continue;

			for (int dir = 0; dir < 5; dir++)
			{
				if (!inst->HasNeighbor(v, dir))
					continue;
				
				if (pass[a][v][dir].first_variable == 0)
					continue;

				int u = inst->GetNeighbor(v, dir);

				int star_t = pass[a][v][dir].first_timestep;
				int end_t = pass[a][v][dir].last_timestep + 1;

				for (int t = star_t; t < end_t; t++)
				{
					int at_var = at[a][v].first_variable + (t - at[a][v].first_timestep);
					int pass_var = pass[a][v][dir].first_variable + (t - pass[a][v][dir].first_timestep);
					int neib_var = at[a][u].first_variable + (t + 1 - at[a][u].first_timestep);

					int v2 = VarToID(at_var, true, vertex_id, dict);
					int u1 = VarToID(neib_var, false, vertex_id, dict);

					//g_theory->newEdge(v2, u1, pass_var);
					if (cnf_file.compare("") != 0)
						cnf_printable << "edge " << a << " " << v2 << " " << u1 << " " << pass_var << "\n";
				}
			}
		}

		// reachability requirement
		int at_start_var = at[a][inst->map[inst->agents[a].start.x][inst->agents[a].start.y]].first_variable;
		_MAPFSAT_TEGAgent AV_goal = at[a][inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y]];
		int at_goal_var = AV_goal.first_variable + (AV_goal.last_timestep - AV_goal.first_timestep);

		int start_v = VarToID(at_start_var, false, vertex_id, dict);
		int goal_v = VarToID(at_goal_var, true, vertex_id, dict);

		//g_theory->reaches(start_v, goal_v, lit);
		if (cnf_file.compare("") != 0)
			cnf_printable << "reach " << a << " " << start_v << " " << goal_v << " " << lit << "\n";
		AddClause(vector<int> {lit});
		nr_clauses_unit++;
		lit++;
	}

	return lit;
}

int _MAPFSAT_SMT::CreateMove_Graph_MonosatShift(int lit)
{
	// GraphTheorySolver_long g_theory = newGraph((SolverPtr)SAT_solver);
	if (cnf_file.compare("") != 0)
		cnf_printable << "digraph int 0 0 0\n";

	int vertex_id = 0;
	unordered_map<int, int> dict;

	for (int v = 0; v < vertices; v++)
	{
		if (shift_times_start[v] == -1)
			continue; 
		for (int t = shift_times_start[v]; t <= shift_times_end[v]; t++)
		{
			for (int dir = 0; dir < 5; dir++)
			{
				if (!inst->HasNeighbor(v, dir))
					continue;

				int u = inst->GetNeighbor(v, dir);

				size_t ind = find(shift[v][dir].timestep.begin(), shift[v][dir].timestep.end(), t) - shift[v][dir].timestep.begin();

				if (ind != shift[v][dir].timestep.size() && shift[v][dir].timestep[ind] == t)
				{
					int shift_var = shift[v][dir].first_varaible + ind;

					int v_id = ((v+t)*(v+t+1)/2) + t; // Cantor pairing function
					int node1 = VarToID(v_id, false, vertex_id, dict);
					int u_id = ((u+t+1)*(u+t+2)/2)+t+1; // Cantor pairing function, u is reached at t+1
					int node2 = VarToID(u_id, false, vertex_id, dict);

					//g_theory->newEdge(v2, u1, pass_var);
					if (cnf_file.compare("") != 0)
						cnf_printable << "edge 0 " << node1 << " " << node2 << " " << shift_var << "\n";
				}
			}
		}
	}

	// reachability requirement
	for (int a = 0; a < agents; a++)
	{
		int v_start = inst->map[inst->agents[a].start.x][inst->agents[a].start.y];
		int v_goal = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];

		int t = 0;
		int v_id = ((v_start+t)*(v_start+t+1)/2) + t; // Cantor pairing function
		int node1 = VarToID(v_id, false, vertex_id, dict);

		t = inst->LastTimestep(a, v_goal, max_timestep, delta, cost_function);
		int u_id = ((v_goal+t)*(v_goal+t+1)/2)+t; // Cantor pairing function, u is reached at t+1
		int node2 = VarToID(u_id, false, vertex_id, dict);

		//g_theory->reaches(node1, node2, lit);
		if (cnf_file.compare("") != 0)
			cnf_printable << "reach 0 " << node1 << " " << node2 << " " << lit << "\n";
		AddClause(vector<int> {lit});
		nr_clauses_unit++;
		lit++;
	}

	return lit;
}

/****************************/
// MARK: aux
/****************************/

void _MAPFSAT_SMT::AddClause(vector<int> clause)
{
	nr_clauses++;
	if (cnf_file.compare("") != 0)
	{
		for (size_t i = 0; i < clause.size(); i++)
			cnf_printable << clause[i] << " ";
		cnf_printable << "0\n";
	}
}

void _MAPFSAT_SMT::CreateSolver()
{
	// using only binary for now
}

void _MAPFSAT_SMT::ReleaseSolver()
{
	// using only binary for now
}

int _MAPFSAT_SMT::InvokeSolverImplementation(int timelimit)
{
	// working only with binary for now
	// setTimeLimit((SolverPtr)SAT_solver, (timelimit/1000) + 1);
	// int ret = solveLimited((SolverPtr)SAT_solver); // 0 = SAT; 1 = UNSAT; 2 = timeout

	stringstream exec;
	exec << "timeout " << (timelimit/1000) +1
		<< " ./libs/monosat" 						
		<< " -witness-file=tmp.out"					// -witness to print assignment
		//<< " -cpu-lim=" << (timelimit/1000) +1	// cpu limit (in s) is unrealiable
		<< " tmp.cnf"
		<< " > /dev/null";							// do not care about stdout 

	int ret = system(exec.str().c_str());

	if ((print_plan || keep_plan || lazy_const == 2) && ret == 2560)
	{
		ifstream input("tmp.out");
		if (!input.is_open())
		{
			cerr << "could not open result file tmp.out" << endl;
			return -1;
		}
		string line;
		getline(input, line);
		vector<bool> eval = vector<bool>(nr_vars);

		if (line.compare("") != 0)
		{
			stringstream ssline(line);
			string part;
			vector<string> parsed_line;
			while (getline(ssline, part, ' '))
			{
				if (part.compare("v") == 0)
					continue;
				if (part.compare("0") == 0)
					continue;
				int var = atoi(part.c_str());
				eval[abs(var)-1] = (var > 0) ? true : false;
			}
		}
		input.close();

		plan = vector<vector<int> >(agents, vector<int>(max_timestep));

		for (int a = 0; a < agents; a++)
		{
			int v = inst->map[inst->agents[a].start.x][inst->agents[a].start.y];
			plan[a][0] = v;

			for (int t = 1; t < max_timestep; t++)
			{
				v = GetNextVertex(eval, a, v, t);
				plan[a][t] = v;
			}
		}

		if (cost_function == 2)
			NormalizePlan();
	}

	return (ret == 2560) ? 0 : 1;
}

int _MAPFSAT_SMT::VarToID(int var, bool duplicate, int& freshID, unordered_map<int, int>& dict)
{
	if (duplicate)
		var = -var;
	
	if (dict.find(var) == dict.end())
	{
		dict[var] = freshID;
		freshID++;
	}

	return dict[var];
}

int _MAPFSAT_SMT::GetNextVertex(vector<bool>& eval, int a, int v, int t)
{
	if (v == -1)
		return -1;

	if (variables == 2) // decide using pass, ghost agents can create swapping conflicts
	{
		int leave_t = t-1;
		for (int dir = 0; dir < 5; dir++)
		{
			if (!inst->HasNeighbor(v,dir))
				continue;
			int u = inst->GetNeighbor(v, dir);
			if (pass[a][v][dir].first_variable == 0)
				continue;
			if (pass[a][v][dir].first_timestep > leave_t || pass[a][v][dir].last_timestep < leave_t)
				continue;

			int pass_var = pass[a][v][dir].first_variable + (leave_t - pass[a][v][dir].first_timestep);
			int at_var = at[a][u].first_variable + (t - at[a][u].first_timestep);		 // no need to check existence, it has to exists, since pass exists

			if (eval[pass_var-1] && eval[at_var-1])
				return u;
		}
	}

	if (variables == 3) // decide using shift
	{
		int leave_t = t-1;
		for (int dir = 0; dir < 5; dir++)
		{
			if (!inst->HasNeighbor(v,dir))
				continue;
			int u = inst->GetNeighbor(v, dir);
			
			size_t ind = find(shift[v][dir].timestep.begin(), shift[v][dir].timestep.end(), leave_t) - shift[v][dir].timestep.begin();

			if (ind == shift[v][dir].timestep.size() || shift[v][dir].timestep[ind] != leave_t)
				continue;

			int shift_var = shift[v][dir].first_varaible + ind;

			if (eval[shift_var-1])
				return u;
		}
	}

	// there is no next vertex
	// under SoC, agents that reached goal are no longer represented
	// but still cause collisions using preprocessing
	return -1;
}