#include "solver_common.hpp"

// hide includes form user
#include "../externals/cadical.hpp" // https://github.com/arminbiere/cadical

using namespace std;

/****************************/
// MARK: constructor
/****************************/

/** Constructor of _MAPFSAT_SAT.
*
* @param var variables to be use. Possible values 1 = at, 2 = pass, 3 = shift.
* @param cost optimized cost function. Possible values 1 = mks, 2 = soc.
* @param moves allowed movement. Possible values 1 = parallel, 2 = pebble.
* @param lazy eager or lazy solving of conflicts. Possible values 1 = eager, 2 = lazy.
* @param dupli allow of forbid duplicating agents. Possible values 1 = forbid, 2 = allow.
* @param solver SAT solver to be used. Possible values 1 = CaDiCaL, 2 = monosat.
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
	solver_to_use = solver; 	// 1 = CaDiCaL, 	2 = monosat

	assert(variables == 1 || variables == 2 || variables == 3);
	assert(cost_function == 1 || cost_function == 2);
	assert(movement == 1 || movement == 2);
	assert(lazy_const == 1 || lazy_const == 2);
	assert(duplicates == 1 || duplicates == 2);
	assert(solver_to_use == 1);
};

/****************************/
// MARK: formula
/****************************/

int _MAPFSAT_SAT::CreateFormula(int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;
	int lit = nr_vars; 	// is 1 on first call
	auto start = chrono::high_resolution_clock::now();

	/********************/
	/* create variables */
	/********************/
	if (first_try)	// varaibles already exist on second try of incremental solve
	{
		lit = CreateAt(lit, timesteps);
		if (variables == 2)
			lit = CreatePass(lit, timesteps);
		if (variables == 3)
			lit = CreateShift(lit, timesteps);
	}

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

	if (!first_try)		// movment clauses already exist
		return lit;

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
	
	/************/
	/* movement */
	/************/
	if (duplicates == 1)
		lit = CreateMove_NoDuplicates(lit);

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

/****************************/
// MARK: aux
/****************************/

void _MAPFSAT_SAT::AddClause(vector<int> clause)
{
	nr_clauses++;
	for (size_t i = 0; i < clause.size(); i++)
		((CaDiCaL::Solver*)SAT_solver)->add(clause[i]);
	((CaDiCaL::Solver*)SAT_solver)->add(0);

	if (cnf_file.compare("") != 0)
	{
		for (size_t i = 0; i < clause.size(); i++)
			cnf_printable << clause[i] << " ";
		cnf_printable << "0\n";
	}
}

void _MAPFSAT_SAT::CreateSolver()
{
	SAT_solver = new CaDiCaL::Solver;
}

void _MAPFSAT_SAT::ReleaseSolver()
{
	delete (CaDiCaL::Solver*)SAT_solver;
	SAT_solver = NULL;
}

int _MAPFSAT_SAT::InvokeSolverImplementation(int timelimit)
{
	bool ended = false;
	thread waiting_thread = thread(WaitForTerminate, timelimit, SAT_solver, ref(ended));
	
	int ret = ((CaDiCaL::Solver*)SAT_solver)->solve(); // Start solver // 20 = UNSAT; 10 = SAT; 0 = UNKNOWN (reached through terminate)

	ended = true;
	waiting_thread.join();

	if ((print_plan || keep_plan || lazy_const == 2) && ret == 10)	// create plan from variables
	{
		plan = vector<vector<int> >(agents, vector<int>(max_timestep));

		for (int a = 0; a < agents; a++)
		{
			int v = inst->map[inst->agents[a].start.x][inst->agents[a].start.y];
			plan[a][0] = v;

			for (int t = 1; t < max_timestep; t++)
			{
				v = GetNextVertex(a, v, t);
				plan[a][t] = v;
			}
		}

		if (cost_function == 2)
			NormalizePlan();
	}

	return (ret == 10) ? 0 : 1;
}

void _MAPFSAT_SAT::WaitForTerminate(int time_left_ms, void* solver, bool& ended)
{
	while (time_left_ms > 0)
	{
		if (ended)
			return;

		this_thread::sleep_for(std::chrono::milliseconds(50));
		time_left_ms -= 50;
	}

	if (ended)
		return;
		
	((CaDiCaL::Solver*)solver)->terminate();	// Trusting in CaDiCaL implementation
}

int _MAPFSAT_SAT::GetNextVertex(int a, int v, int t)
{
	if (v == -1)
		return -1;

	if (variables == 1) // check just at, no possible swapping conflict caused by ghost agents
	{
		for (int dir = 0; dir < 5; dir++)
		{
			if (!inst->HasNeighbor(v,dir))
				continue;
			int u = inst->GetNeighbor(v, dir);
			if (at[a][u].first_variable == 0)
				continue;
			if (at[a][u].first_timestep > t || at[a][u].last_timestep < t)
				continue;

			int neib_var = at[a][u].first_variable + (t - at[a][u].first_timestep);
			if (((CaDiCaL::Solver*)SAT_solver)->val(neib_var) > 0)
				return u;
		}
	}

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
			if (((CaDiCaL::Solver*)SAT_solver)->val(pass_var) > 0)
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
			if (at[a][u].first_variable == 0)
				continue;
			if (at[a][u].first_timestep > t || at[a][u].last_timestep < t)
				continue;
			
			size_t ind = find(shift[v][dir].timestep.begin(), shift[v][dir].timestep.end(), leave_t) - shift[v][dir].timestep.begin();

			if (ind == shift[v][dir].timestep.size() || shift[v][dir].timestep[ind] != leave_t)
				continue;

			int shift_var = shift[v][dir].first_varaible + ind;

			if (((CaDiCaL::Solver*)SAT_solver)->val(shift_var) > 0)
				return u;
		}
	}

	// there is no next vertex
	// under SoC, agents that reached goal are no longer represented
	// but still cause collisions using preprocessing
	return -1;
}