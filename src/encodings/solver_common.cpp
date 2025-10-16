#include "solver_common.hpp"

// hide includes form user
#pragma GCC diagnostic push // external libs have some warnings, lets ignore them
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "../externals/pblib/pb2cnf.h" // https://github.com/master-keying/pblib
#pragma GCC diagnostic pop	// do not ignore warnings in our code!

using namespace std;

/****************************/
// MARK: before solving
/****************************/

void _MAPFSAT_ISolver::SetData(_MAPFSAT_Instance* i, _MAPFSAT_Logger* l, int to, string cf, bool q, bool p, bool av)
{
	inst = i;
	log = l;
	timeout = to; // in s
	quiet = q;
	print_plan = p;
	use_avoid = av;
	cnf_file = cf;

	if (quiet)
		print_plan = false;

	at = NULL;
	pass = NULL;
	shift = NULL;
	shift_times_start = NULL;
	shift_times_end = NULL;
};

void _MAPFSAT_ISolver::PrintSolveDetails(int time_left)
{
	if (quiet)
		return;
	
	cout << "Currently solving" << endl;
	cout << "Instance name: " << inst->scen_name << endl;
	cout << "Map name: " << inst->map_name << endl;
	cout << "Encoding used: " << solver_name << endl;
	cout << "SAT solver used: " << ((solver_to_use == 1) ? "CaDiCaL" : "Monosat") << endl;
	cout << "Optimizing function: " << ((cost_function == 1) ? "makespan" : "sum of costs") << endl;
	cout << "Number of agents: " << agents << endl;
	cout << "Mks LB: " << inst->GetMksLB(agents) << endl;
	cout << "SoC LB: " << inst->GetSocLB(agents) << endl;
	cout << "Delta: " << delta << endl;
	cout << "Remaining time: " << time_left << " [ms]" << endl;
	cout << endl;
};

/********************************/
// MARK: solving MAPF
/********************************/

int _MAPFSAT_ISolver::Solve(int ags, int input_delta, bool oneshot, bool keep)
{
	delta = input_delta;
	int time_left = timeout * 1000; // given in s, tranfer to ms
	long long building_time = 0;
	long long solving_time = 0;
	solver_calls = 0;
	nr_clauses = 0;
	nr_clauses_move = 0;
	nr_clauses_conflict = 0;
	nr_clauses_soc = 0;
	nr_clauses_assumption = 0;
	keep_plan = keep;
	cnf_printable.clear();	// store cnf here if specified to print in cnf_file
	vertex_conflicts.clear();
	swap_conflicts.clear();
	pebble_conflicts.clear();

	SAT_solver = NULL;
	at = NULL;
	pass = NULL;
	shift = NULL;
	agents = ags;
	vertices = inst->number_of_vertices;

	first_try = true;

	while (true)
	{
		int res = 1; // 0 = sat, 1 = unsat
		conflicts_present = false;
		long long current_building_time = 0;
		long long current_solving_time = 0;
		PrintSolveDetails(time_left);

		if (SAT_solver == NULL)	// in case of incremental solving
			CreateSolver();

		// create formula
		auto start = chrono::high_resolution_clock::now();
		nr_vars = CreateFormula(time_left);
		auto stop = chrono::high_resolution_clock::now();
		if (TimesUp(start, stop, time_left))
			return 1;

		current_building_time = chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		building_time += current_building_time;
		time_left -= current_building_time;

		// solve formula
		start = chrono::high_resolution_clock::now();
		res = InvokeSolver(time_left + 100);
		stop = chrono::high_resolution_clock::now();
		if (TimesUp(start, stop, time_left))
			return 1;

		current_solving_time = chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		solving_time += current_solving_time;
		time_left -= current_solving_time;
		
		// log statistics
		log->nr_vars = nr_vars;
		log->nr_clauses = nr_clauses;
		log->nr_clauses_move = nr_clauses_move;
		log->nr_clauses_conflict = nr_clauses_conflict;
		log->nr_clauses_soc = nr_clauses_soc;
		log->nr_clauses_assumption = nr_clauses_assumption;
		log->building_time = building_time;
		log->solving_time = solving_time;
		log->solution_mks = inst->GetMksLB(agents) + delta;
		log->solution_soc = (cost_function == 1) ? 0 : inst->GetSocLB(agents) + delta;
		log->solver_calls = solver_calls;
		log->res = res;

		// what to do next?
		if (res == 0 && lazy_const == 2 && conflicts_present) // if there are still conflicts, add contraints
		{
			cout << "conflicts present, go again" << endl;
			first_try = false;
			continue;
		}

		// prepare for next iteration of solving, delat++
		CleanUp();	
		ReleaseSolver();
		cnf_printable.clear();
		first_try = true;
		delta++;

		/*if (res == 0 && lazy_const == 2 && !conflicts_present) // if there are no more conflicts, return success
			return 0;

		if (res == 0 && lazy_const == 1) // success
			return 0;*/

		if (res == 0) 	// success
			return 0;

		if (res == -1) 	// something went horribly wrong with the solver!
			return 1;
		
		if (oneshot)	// no solution with the given delta, do not optimize, return no sol
			return -1;
	}

	return 0;	// should not get here
}

/****************************/
// MARK: create varaibles
/****************************/

int _MAPFSAT_ISolver::CreateAt(int lit, int timesteps)
{
	at = new _MAPFSAT_TEGAgent*[agents];

	for (int a = 0; a < agents; a++)
	{
		at[a] = new _MAPFSAT_TEGAgent[vertices];
		for (int v = 0; v < vertices; v++)
		{
			if (inst->FirstTimestep(a, v) <= inst->LastTimestep(a, v, timesteps, delta, cost_function))
			{
				at[a][v].first_variable = lit;
				at[a][v].first_timestep = inst->FirstTimestep(a, v);
				at[a][v].last_timestep = inst->LastTimestep(a, v, timesteps, delta, cost_function);
				lit += at[a][v].last_timestep - at[a][v].first_timestep + 1;
				//cout << "create at a, v " << a << ", " << v;
				//cout << " variables from ID " << at[a][v].first_variable;
				//cout << " timesteps from, to " << at[a][v].first_timestep << ", " << at[a][v].last_timestep << endl;
			}
			else
			{
				at[a][v].first_variable = 0;
			}
		}
	}

	max_timestep = timesteps;
	at_vars = lit; // at vars always start at 1
	return lit;
}

int _MAPFSAT_ISolver::CreatePass(int lit, int timesteps)
{
	pass = new _MAPFSAT_TEGAgent**[agents];

	for (int a = 0; a < agents; a++)
	{
		pass[a] = new _MAPFSAT_TEGAgent*[vertices];
		for (int v = 0; v < vertices; v++)
		{
			pass[a][v] = new _MAPFSAT_TEGAgent[5]; // 5 directions from a vertex
			for (int dir = 0; dir < 5; dir++)
			{
				if (!inst->HasNeighbor(v, dir))
				{
					pass[a][v][dir].first_variable = 0;
					continue;
				}
				if (inst->FirstTimestep(a, v) < inst->LastTimestep(a, inst->GetNeighbor(v, dir), timesteps, delta, cost_function)) // !!! might be wrong
				{
					pass[a][v][dir].first_variable = lit;
					pass[a][v][dir].first_timestep = inst->FirstTimestep(a, v);
					pass[a][v][dir].last_timestep = inst->LastTimestep(a, inst->GetNeighbor(v, dir), timesteps, delta, cost_function) - 1;
					lit += pass[a][v][dir].last_timestep - pass[a][v][dir].first_timestep + 1;
					//cout << "create pass a, v, dir " << a << " " << v << " " << dir;
					//cout << " variables from ID " << pass[a][v][dir].first_variable;
					//cout << " timesteps from, to " << pass[a][v][dir].first_timestep << " " << pass[a][v][dir].last_timestep << endl;
				}
				else
				{
					pass[a][v][dir].first_variable = 0;
				}
			}
		}
	}

	return lit;
}

int _MAPFSAT_ISolver::CreateShift(int lit, int timesteps)
{
	shift = new _MAPFSAT_Shift*[vertices];
	shift_times_start = new int[vertices];
	shift_times_end = new int[vertices];

	for (int v = 0; v < vertices; v++)
	{
		shift[v] = new _MAPFSAT_Shift[5];	// 5 directions from a vertex
		shift_times_start[v] = -1;
		shift_times_end[v] = -1;
		for (int dir = 0; dir < 5; dir++)
		{
			shift[v][dir].first_varaible = lit;	// does not necessary mean that there are any varaibles, check lenght of .timestep
			if (!inst->HasNeighbor(v, dir))
					continue;
			for (int t = 0; t < timesteps; t++)
			{
				for (int a = 0; a < agents; a++)
				{	
					// create shift variable only for possition only if any agent can traverse there
					if (inst->FirstTimestep(a, v) <= t && inst->LastTimestep(a, inst->GetNeighbor(v, dir), timesteps, delta, cost_function) > t)
					{
						shift[v][dir].timestep.push_back(t);
						lit++;
						//cout << "create shift v, dir " << v << " " << dir;
						//cout << " variable ID " << lit-1;
						//cout << " timestep " << t << endl;

						if (shift_times_start[v] == -1 || shift_times_start[v] > t)
							shift_times_start[v] = t;
						if (shift_times_end[v] < t)
							shift_times_end[v] = t;

						break;
					}
				}
			}
		}
	}
	max_timestep = timesteps;

	return lit;
}

/****************************/
// MARK: create constraints
/****************************/

void _MAPFSAT_ISolver::CreatePossition_Start()
{
	for (int a = 0; a < agents; a++)
	{
		int start_var = at[a][inst->map[inst->agents[a].start.x][inst->agents[a].start.y]].first_variable;
		AddClause(vector<int> {start_var});
		nr_clauses_assumption++;
	}
}

void _MAPFSAT_ISolver::CreatePossition_Goal()
{
	for (int a = 0; a < agents; a++)
	{
		_MAPFSAT_TEGAgent AV_goal = at[a][inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y]];
		int goal_var = AV_goal.first_variable + (AV_goal.last_timestep - AV_goal.first_timestep);
		AddClause(vector<int> {goal_var});
		nr_clauses_assumption++;
	}
}

void _MAPFSAT_ISolver::CreatePossition_NoneAtGoal()
{
	for (int a = 0; a < agents; a++)
	{
		int v = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];
		
		for (int a2 = 0; a2 < agents; a2++)
		{
			if (at[a2][v].first_variable == 0)
				continue;

			int star_t = max(at[a2][v].first_timestep, at[a][v].last_timestep + 1);
			int end_t = at[a2][v].last_timestep + 1;

			for (int t = star_t; t < end_t; t++)
			{
				//cout << a2 << " can not be at " << v << ", timestep " << t << " becuase " << a << " is in goal there" << endl;
				int a2_var = at[a2][v].first_variable + (t - at[a2][v].first_timestep);
				AddClause(vector<int> {-a2_var});
				nr_clauses_assumption++;
			}
		}
	}
}

void _MAPFSAT_ISolver::CreatePossition_NoneAtGoal_Shift()
{
	for (int a = 0; a < agents; a++)
	{
		int v = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];
		if (shift_times_start[v] == -1)
			continue; 

		for (int t = max(inst->LastTimestep(a, v, max_timestep, delta, cost_function), shift_times_start[v]); t <= shift_times_end[v]; t++)
		{
			for (int dir = 0; dir < 5; dir++)
			{
				if (!inst->HasNeighbor(v, dir))
					continue;

				size_t ind = find(shift[v][dir].timestep.begin(), shift[v][dir].timestep.end(), t) - shift[v][dir].timestep.begin();

				if (ind != shift[v][dir].timestep.size() && shift[v][dir].timestep[ind] == t)
				{
					//cout << "there is no shift from " << v << ", " << dir << " in timestep " << t << " varaible " << shift_var;
					//cout << " because " << a << " has a goal there" << endl;
					int shift_var = shift[v][dir].first_varaible + ind;
					AddClause(vector<int> {-shift_var});
					nr_clauses_assumption++;
				}
			}

		}
	}
}

void _MAPFSAT_ISolver::CreateConf_Vertex()
{
	for (int v = 0; v < vertices; v++)
	{
		for (int a1 = 0; a1 < agents; a1++)
		{
			if (at[a1][v].first_variable == 0)
				continue;
			for (int a2 = a1 + 1; a2 < agents; a2++)
			{
				if (at[a2][v].first_variable == 0)
					continue;
				int star_t = max(at[a1][v].first_timestep, at[a2][v].first_timestep);
				int end_t = min(at[a1][v].last_timestep, at[a2][v].last_timestep) + 1;

				for (int t = star_t; t < end_t; t++)
				{
					//cout << "vertex conflict at vertex " << v << ", timestep " << t << " between " << a1 << " and " << a2 << endl;
					int a1_var = at[a1][v].first_variable + (t - at[a1][v].first_timestep);
					int a2_var = at[a2][v].first_variable + (t - at[a2][v].first_timestep);
					AddClause(vector<int> {-a1_var, -a2_var});
					nr_clauses_conflict++;
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateConf_Swapping_At()
{
	for (int v = 0; v < vertices; v++)
	{
		for (int dir = 1; dir < 5; dir++) // ignore waiting, ie. selfloops
		{
			if (!inst->HasNeighbor(v, dir))
				continue;

			int u = inst->GetNeighbor(v, dir);

			for (int a1 = 0; a1 < agents; a1++)
			{
				if (at[a1][v].first_variable == 0 || at[a1][u].first_variable == 0)
					continue;

				for (int a2 = a1 + 1; a2 < agents; a2++)
				{
					if (at[a2][u].first_variable == 0 || at[a2][v].first_variable == 0)
						continue;

					int star_t = max({at[a1][v].first_timestep, at[a1][u].first_timestep - 1, at[a2][u].first_timestep, at[a2][v].first_timestep - 1});
					int end_t = min({at[a1][v].last_timestep, at[a1][u].last_timestep - 1, at[a2][u].last_timestep, at[a2][v].last_timestep - 1}) + 1;

					for (int t = star_t; t < end_t; t++)
					{
						//cout << "swapping conflict between vertices (" << v << "," << u << "), timestep " << t << " between " << a1 << " and " << a2 << endl;
						int a1_v_var = at[a1][v].first_variable + (t - at[a1][v].first_timestep);
						int a1_u_var = at[a1][u].first_variable + (t + 1 - at[a1][u].first_timestep);
						int a2_v_var = at[a2][v].first_variable + (t + 1 - at[a2][v].first_timestep);
						int a2_u_var = at[a2][u].first_variable + (t - at[a2][u].first_timestep);
						AddClause(vector<int> {-a1_v_var, -a1_u_var, -a2_v_var, -a2_u_var});
						nr_clauses_conflict++;
					}
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateConf_Swapping_Pass()
{
	for (int v = 0; v < vertices; v++)
	{
		for (int dir = 1; dir < 5; dir++)
		{
			if (!inst->HasNeighbor(v, dir))
				continue;
			for (int a1 = 0; a1 < agents; a1++)
			{
				if (pass[a1][v][dir].first_variable == 0)
					continue;
				for (int a2 = a1 + 1; a2 < agents; a2++)
				{
					int u = inst->GetNeighbor(v, dir);
					int op_dir = inst->OppositeDir(dir);

					if (pass[a2][u][op_dir].first_variable == 0)
						continue;

					int star_t = max(pass[a1][v][dir].first_timestep, pass[a2][u][op_dir].first_timestep);
					int end_t = min(pass[a1][v][dir].last_timestep, pass[a2][u][op_dir].last_timestep) + 1;

					for (int t = star_t; t < end_t; t++)
					{
						//cout << "swapping conflict at edge (" << v << "," << u << "), timestep " << t << " between " << a1 << " and " << a2 << endl;
						int a1_var = pass[a1][v][dir].first_variable + (t - pass[a1][v][dir].first_timestep);
						int a2_var = pass[a2][u][op_dir].first_variable + (t - pass[a2][u][op_dir].first_timestep);
						AddClause(vector<int> {-a1_var, -a2_var});
						nr_clauses_conflict++;
					}
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateConf_Swapping_Shift()
{
	// No two opposite shifts at the same time
	for (int v = 0; v < vertices; v++)
	{
		for (int dir = 2; dir < 4; dir++)	// no need to check other directions to prevent multiple clauses
		{
			if (!inst->HasNeighbor(v, dir))
				continue;
			
			for (size_t t_ind = 0; t_ind < shift[v][dir].timestep.size(); t_ind++)
			{
				int op_dir = inst->OppositeDir(dir);
				int u = inst->GetNeighbor(v, dir);
				size_t ind = find(shift[u][op_dir].timestep.begin(), shift[u][op_dir].timestep.end(), shift[v][dir].timestep[t_ind]) - shift[u][op_dir].timestep.begin();

				if (ind == shift[u][op_dir].timestep.size() || shift[u][op_dir].timestep[ind] != shift[v][dir].timestep[t_ind]) // did not find t in opposite dir shift
					continue;
				
				//cout << "swapping conflict at edge (" << v << "," << u << "), timestep " << t << " using shift" << endl;
				int shift1_var = shift[v][dir].first_varaible + t_ind;
				int shift2_var = shift[u][op_dir].first_varaible + ind;
				AddClause(vector<int> {-shift1_var, -shift2_var});
				nr_clauses_conflict++;
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateConf_Pebble_At()
{
	for (int v = 0; v < vertices; v++)
	{
		for (int a1 = 0; a1 < agents; a1++)
		{
			if (at[a1][v].first_variable == 0)
				continue;
			
			for (int a2 = 0; a2 < agents; a2++)
			{
				if (a1 == a2)
					continue;
				if (at[a2][v].first_variable == 0)
					continue;

				int star_t = max(at[a1][v].first_timestep, at[a2][v].first_timestep + 1);
				int end_t = min(at[a1][v].last_timestep, at[a2][v].last_timestep + 1) + 1;

				for (int t = star_t; t < end_t; t++)
				{
					//cout << "pebble conflict at vertex " << v << ", timestep " << t << " between " << a1 << " and " << a2 << endl;
					int a1_var = at[a1][v].first_variable + (t - at[a1][v].first_timestep);
					int a2_var = at[a2][v].first_variable + (t - 1 - at[a2][v].first_timestep);
					AddClause(vector<int> {-a1_var, -a2_var});
					nr_clauses_conflict++;
				}

			}

		}
	}
}

void _MAPFSAT_ISolver::CreateConf_Pebble_Pass()
{
	// Pass(a1,t,u,v) -> forall a2: -At(a2,t,v)
	for (int v = 0; v < vertices; v++)
	{
		for (int dir = 1; dir < 5; dir++) // ignore waiting, ie. selfloops
		{
			if (!inst->HasNeighbor(v, dir))
				continue;
			for (int a1 = 0; a1 < agents; a1++)
			{
				if (pass[a1][v][dir].first_variable == 0)
					continue;
				for (int a2 = 0; a2 < agents; a2++)
				{
					if (a1 == a2)
						continue;

					int u = inst->GetNeighbor(v, dir);
					if (at[a2][u].first_variable == 0)
						continue;

					int star_t = max(pass[a1][v][dir].first_timestep, at[a2][u].first_timestep);
					int end_t = min(pass[a1][v][dir].last_timestep, at[a2][u].last_timestep) + 1;

					for (int t = star_t; t < end_t; t++)
					{
						//cout << "pebble conflict at edge (" << v << "," << u << "), timestep " << t << " between moving agent " << a1 << " and " << a2 << endl;
						int a1_var = pass[a1][v][dir].first_variable + (t - pass[a1][v][dir].first_timestep);
						int a2_var = at[a2][u].first_variable + (t - at[a2][u].first_timestep);
						AddClause(vector<int> {-a1_var, -a2_var});
						nr_clauses_conflict++;
					}
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateConf_Pebble_Shift()
{
	// original paper: if shift v->u, then there is also shift u->u (vertex conflict will take care of it)
	// our implementation: if shift v->u, then there is no shift u->w (shift u->u is possible)
	for (int v = 0; v < vertices; v++)
	{
		for (int dir = 0; dir < 5; dir++)
		{
			if (!inst->HasNeighbor(v, dir))
				continue;
			
			for (size_t t_ind = 0; t_ind < shift[v][dir].timestep.size(); t_ind++)
			{
				int u = inst->GetNeighbor(v, dir);
				int shift1_var = shift[v][dir].first_varaible + t_ind;

				for (int u_dir = 1; u_dir < 5; u_dir++) // do not check waiting direction
				{
					size_t ind = find(shift[u][u_dir].timestep.begin(), shift[u][u_dir].timestep.end(), shift[v][dir].timestep[t_ind]) - shift[u][u_dir].timestep.begin();

					if (ind == shift[u][u_dir].timestep.size() || shift[u][u_dir].timestep[ind] != shift[v][dir].timestep[t_ind]) // did not find t in neighboring shift
						continue;
					
					//cout << "pebble conflict at edge (" << v << "," << u << "), timestep " << shift[v][dir].timestep[t_ind] << " direction " << u_dir << " using shift" << endl;
					int shift2_var = shift[u][u_dir].first_varaible + ind;
					AddClause(vector<int> {-shift1_var, -shift2_var});
					nr_clauses_conflict++;
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateMove_NoDuplicates()
{
	for (int a = 0; a < agents; a++)
	{
		for (int v = 0; v < vertices; v++)
		{
			if (at[a][v].first_variable == 0)
				continue;
			for (int u = v+1; u < vertices; u++)
			{
				if (at[a][u].first_variable == 0)
					continue;

				int star_t = max(at[a][v].first_timestep, at[a][u].first_timestep);
				int end_t = min(at[a][v].last_timestep, at[a][u].last_timestep) + 1;

				for (int t = star_t; t < end_t; t++)
				{
					//cout << "forbid agent " << a << " at vertices " << v << ", " << u << " at time " << t << endl;
					int v_var = at[a][v].first_variable + (t - at[a][v].first_timestep);
					int u_var = at[a][u].first_variable + (t - at[a][u].first_timestep);
					AddClause(vector<int> {-v_var, -u_var});
					nr_clauses_move++;
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateMove_NextVertex_At()
{
	for (int a = 0; a < agents; a++)
	{
		for (int v = 0; v < vertices; v++)
		{
			if (at[a][v].first_variable == 0)
				continue;

			int star_t = at[a][v].first_timestep;
			int end_t = at[a][v].last_timestep + 1;

			for (int t = star_t; t < end_t; t++)
			{
				int neib_t = t+1;
				vector<int> neibs;
				for (int dir = 0; dir < 5; dir++)
				{
					if (!inst->HasNeighbor(v,dir))
						continue;
					int u = inst->GetNeighbor(v, dir);
					if (at[a][u].first_variable == 0)
						continue;
					if (at[a][u].first_timestep <= neib_t && at[a][u].last_timestep >= neib_t)
					{
						//cout << a << " can move from " << v << " into " << u << " at timestep " << t << endl;
						int neib_var = at[a][u].first_variable + (neib_t - at[a][u].first_timestep);
						neibs.push_back(neib_var);
					}
				}

				if (neibs.empty())
					continue;
				
				int at_var = at[a][v].first_variable + (t - at[a][v].first_timestep);
				neibs.push_back(-at_var);

				AddClause(neibs);
				nr_clauses_move++;
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateMove_EnterVertex_Pass()
{
	for (int v = 0; v < vertices; v++)
	{
		for (int dir = 0; dir < 5; dir++)
		{
			if (!inst->HasNeighbor(v, dir))
				continue;
			for (int a = 0; a < agents; a++)
			{
				if (pass[a][v][dir].first_variable == 0)
					continue;
				int star_t = pass[a][v][dir].first_timestep;
				int end_t = pass[a][v][dir].last_timestep + 1;

				for (int t = star_t; t < end_t; t++)
				{
					int u = inst->GetNeighbor(v, dir);
					int pass_var = pass[a][v][dir].first_variable + (t - pass[a][v][dir].first_timestep);
					int at_var = at[a][u].first_variable + (t + 1 - at[a][u].first_timestep);

					//cout << "moving agent " << a << " over (" << v << ", " << u << ") at timestep " << t << " will lead to " << u << endl;
					AddClause(vector<int> {-pass_var, at_var});
					nr_clauses_move++;
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateMove_LeaveVertex_Pass()
{
	for (int v = 0; v < vertices; v++)
	{
		for (int dir = 0; dir < 5; dir++)
		{
			if (!inst->HasNeighbor(v, dir))
				continue;
			for (int a = 0; a < agents; a++)
			{
				if (pass[a][v][dir].first_variable == 0)
					continue;
				int star_t = pass[a][v][dir].first_timestep;
				int end_t = pass[a][v][dir].last_timestep + 1;

				for (int t = star_t; t < end_t; t++)
				{
					int pass_var = pass[a][v][dir].first_variable + (t - pass[a][v][dir].first_timestep);
					int at_var = at[a][v].first_variable + (t - at[a][v].first_timestep);

					//cout << "agent " << a << " is moving from << v << in direction " << dir << " at timestep " << t << " means he was in " << v << endl;
					AddClause(vector<int> {-pass_var, at_var});
					nr_clauses_move++;
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateMove_NextEdge_Pass()
{
	for (int a = 0; a < agents; a++)
	{
		for (int v = 0; v < vertices; v++)
		{
			if (at[a][v].first_variable == 0)
				continue;

			int star_t = at[a][v].first_timestep;
			int end_t = at[a][v].last_timestep + 1;

			for (int t = star_t; t < end_t; t++)
			{
				vector<int> neibs;
				for (int dir = 0; dir < 5; dir++)
				{
					if (pass[a][v][dir].first_variable == 0)
						continue;
					if (pass[a][v][dir].first_timestep <= t && pass[a][v][dir].last_timestep >= t)
					{
						//cout << a << " can move from " << v << " in " << dir << " at timestep " << t << endl;
						int pass_var = pass[a][v][dir].first_variable + (t - pass[a][v][dir].first_timestep);
						neibs.push_back(pass_var);
					}
				}

				if (neibs.empty())
					continue;
				
				int at_var = at[a][v].first_variable + (t - at[a][v].first_timestep);
				neibs.push_back(-at_var);

				AddClause(neibs);
				nr_clauses_move++;
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateMove_ExactlyOne_Shift()
{
	// all shifts going from v sum up to at most 1
	for (int v = 0; v < vertices; v++)
	{
		if (shift_times_start[v] == -1)
			continue; 
		for (int t = shift_times_start[v]; t <= shift_times_end[v]; t++)
		{
			vector<int> vc;
			for (int dir = 0; dir < 5; dir++)
			{
				if (!inst->HasNeighbor(v, dir))
					continue;

				size_t ind = find(shift[v][dir].timestep.begin(), shift[v][dir].timestep.end(), t) - shift[v][dir].timestep.begin();

				if (ind != shift[v][dir].timestep.size() && shift[v][dir].timestep[ind] == t)
				{
					int shift_var = shift[v][dir].first_varaible + ind;
					vc.push_back(shift_var);
					//cout << "there is a shift from " << v << ", " << dir << " in timestep " << t << " varaible " <<  shift_var << endl;
				}
			}

			if (vc.empty())
				continue;
			
			//AddClause(vc); // at least 1 - do not use!!

			for (size_t i = 0; i < vc.size(); i++)
			{
				for (size_t j = i+1; j < vc.size(); j++)
				{
					AddClause(vector<int>{-vc[i], -vc[j]}); // at most 1
					nr_clauses_move++;
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateMove_ExactlyOneIncoming_Shift()
{
	// all shifts going into v sum up to at most 1
	for (int v = 0; v < vertices; v++)
	{
		int min_t = INT_MAX;
		int max_t = -1;

		for (int dir = 0; dir < 5; dir++)
		{
			if (!inst->HasNeighbor(v, dir))
				continue;

			int u = inst->GetNeighbor(v,dir);

			if (shift_times_start[u] == -1)
				continue; 

			min_t = min(min_t, shift_times_start[u]);
			max_t = max(max_t, shift_times_end[u]);
		}

		for (int t = min_t; t <= max_t; t++)
		{
			vector<int> vc;

			for (int dir = 0; dir < 5; dir++)
			{
				if (!inst->HasNeighbor(v, dir))
					continue;

				int u = inst->GetNeighbor(v,dir);
				int op_dir = inst->OppositeDir(dir);

				size_t ind = find(shift[u][op_dir].timestep.begin(), shift[u][op_dir].timestep.end(), t) - shift[u][op_dir].timestep.begin();

				if (ind != shift[u][op_dir].timestep.size() && shift[u][op_dir].timestep[ind] == t)
				{
					int shift_var = shift[u][op_dir].first_varaible + ind;
					vc.push_back(shift_var);
					//cout << "there is a shift from " << u << ", " << op_dir << " in timestep " << t << " varaible " <<  shift_var << endl;
				}

				if (vc.empty())
					continue;

				for (size_t i = 0; i < vc.size(); i++)
				{
					for (size_t j = i+1; j < vc.size(); j++)
					{
						AddClause(vector<int>{-vc[i], -vc[j]}); // at most 1
						nr_clauses_move++;
					}
				}
			}
		}
	}
}

void _MAPFSAT_ISolver::CreateMove_NextVertex_Shift()
{
	
	for (int a = 0; a < agents; a++)
	{
		for (int v = 0; v < vertices; v++)
		{
			if (at[a][v].first_variable == 0)
				continue;

			int star_t = at[a][v].first_timestep;
			int end_t = at[a][v].last_timestep + 1;

			for (int t = star_t; t < end_t; t++)
			{
				int neib_t = t+1;
				for (int dir = 0; dir < 5; dir++)
				{
					if (!inst->HasNeighbor(v,dir))
						continue;
					int u = inst->GetNeighbor(v, dir);
					if (at[a][u].first_variable == 0)
						continue;
					if (at[a][u].first_timestep > neib_t || at[a][u].last_timestep < neib_t)
						continue;
					
					size_t ind = find(shift[v][dir].timestep.begin(), shift[v][dir].timestep.end(), t) - shift[v][dir].timestep.begin();

					if (ind == shift[v][dir].timestep.size() || shift[v][dir].timestep[ind] != t)
						continue;

					int at1_var = at[a][v].first_variable + (t - at[a][v].first_timestep);
					int at2_var = at[a][u].first_variable + (neib_t - at[a][u].first_timestep);
					int shift_var = shift[v][dir].first_varaible + ind;

					//cout << "agent " << a << " is at " << v << " and something is moving to " << u << " at timestep " << t << endl;
					//cout << "agent " << a << " is at " << v << " in " << t << " and at " << u << " in the next timestep, therefore something moved" << endl;

					AddClause(vector<int> {-at1_var, -shift_var, at2_var}); // if at v and v shifts to u then at u in the next timestep
					nr_clauses_move++;
					AddClause(vector<int> {-at1_var, -at2_var, shift_var}); // if at v and at u in next timestep then v shifted to u 
					nr_clauses_move++;
				}
			}
		}
	}
}

int _MAPFSAT_ISolver::CreateConst_LimitSoc(int lit)
{
	vector<int> late_variables;

	for (int a = 0; a < agents; a++)
	{
		int goal_v = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];
		int at_var = at[a][goal_v].first_variable;
		for (int d = 0; d < delta; d++)
		{
			AddClause(vector<int> {at_var + d, lit});	// if agent is not at goal, it is late
			nr_clauses_soc++;
			if (d < delta - 1)
			{
				AddClause(vector<int> {lit, -(lit + 1)});	// if agent is not late at t, it is not late at t+1
				nr_clauses_soc++;
			}
			late_variables.push_back(lit);
			lit++;
		}
	}

	// add constraint on sum of delays
	PB2CNF pb2cnf;
	vector<vector<int> > formula;
	lit = pb2cnf.encodeAtMostK(late_variables, delta, formula, lit) + 1;

	for (size_t i = 0; i < formula.size(); i++)
	{
		AddClause(formula[i]);
		nr_clauses_soc++;
	}

	return lit;
}

int _MAPFSAT_ISolver::CreateConst_LimitSoc_AllAt(int lit)
{
	vector<int> late_variables;

	for (int a = 0; a < agents; a++)
	{
		int goal_v = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];
		int t = at[a][goal_v].first_timestep;
		for (int d = 0; d < delta; d++)
		{
			for (int v = 0; v < vertices; v++)
			{
				if (v == goal_v)
					continue;
				if (at[a][v].first_variable == 0)
					continue;
				if (at[a][v].first_timestep > t + d)
					continue;
				if (at[a][v].last_timestep < t + d)
					continue;
				
				int at_var = at[a][v].first_variable + (t + d - at[a][v].first_timestep);

				//cout << "agent " << a << " might be in " << v << " at timestep " << t + d << endl;
				//cout << "therefore, either " << at_var << " is not true or " << lit << " is true" << endl;

				AddClause(vector<int> {-at_var, lit});	// if agent is somewhere other than at goal, it is late
				nr_clauses_soc++;
			}

			if (d < delta - 1)
			{
				AddClause(vector<int> {lit, -(lit + 1)});	// if agent is not late at t, it is not late at t+1
				nr_clauses_soc++;
			}
			late_variables.push_back(lit);
			lit++;
		}
	}

	// add constraint on sum of delays
	PB2CNF pb2cnf;
	vector<vector<int> > formula;
	lit = pb2cnf.encodeAtMostK(late_variables, delta, formula, lit) + 1;

	for (size_t i = 0; i < formula.size(); i++)
	{
		AddClause(formula[i]);
		nr_clauses_soc++;
	}

	return lit;
}

int _MAPFSAT_ISolver::CreateConst_LimitSoc_Shift(int lit)
{
	vector<int> late_variables;

	for (int a = 0; a < agents; a++)
	{
		int goal_v = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];
		int t = inst->FirstTimestep(a, goal_v);

		for (int d = 0; d < delta; d++)
		{
			vector<int> vc;

			for (int dir = 1; dir < 5; dir++) // ignore waiting shifts
			{
				if (!inst->HasNeighbor(goal_v, dir))
					continue;

				int u = inst->GetNeighbor(goal_v,dir);
				int op_dir = inst->OppositeDir(dir);

				size_t ind = find(shift[u][op_dir].timestep.begin(), shift[u][op_dir].timestep.end(), t+d) - shift[u][op_dir].timestep.begin();

				if (ind != shift[u][op_dir].timestep.size() && shift[u][op_dir].timestep[ind] == t+d)
				{
					int shift_var = shift[u][op_dir].first_varaible + ind;

					//cout << "there is a shift from " << u << " into " << goal_v << " at time " << t+d << " which is a goal vertex of " << a << endl;
					AddClause(vector<int> {-shift_var, lit});	// if agent is somewhere other than at goal, it is late
					nr_clauses_soc++;
				}
			}

			if (d < delta - 1)
			{
				AddClause(vector<int> {lit, -(lit + 1)});	// if agent is not late at t, it is not late at t+1
				nr_clauses_soc++;
			}
			late_variables.push_back(lit);
			lit++;
		}
	}

	// add constraint on sum of delays
	PB2CNF pb2cnf;
	vector<vector<int> > formula;
	lit = pb2cnf.encodeAtMostK(late_variables, delta, formula, lit) + 1;

	for (size_t i = 0; i < formula.size(); i++)
	{
		AddClause(formula[i]);
		nr_clauses_soc++;
	}

	return lit;
}

void _MAPFSAT_ISolver::CreateConf_Vertex_OnDemand()
{
	for (size_t i = 0; i < vertex_conflicts.size(); i++)
	{
		int a1, a2, v, t;
		tie(a1, a2, v, t) = vertex_conflicts[i];

		//cout << "extra vertex conflict at vertex " << v << ", timestep " << t << " between " << a1 << " and " << a2 << endl;
		int a1_var = at[a1][v].first_variable + (t - at[a1][v].first_timestep);
		int a2_var = at[a2][v].first_variable + (t - at[a2][v].first_timestep);
		AddClause(vector<int> {-a1_var, -a2_var});
		nr_clauses_conflict++;
	}
}

void _MAPFSAT_ISolver::CreateConf_Swapping_At_OnDemand()
{
	for (size_t i = 0; i < swap_conflicts.size(); i++)
	{
		int a1, a2, v, u, t;
		tie(a1, a2, v, u, t) = swap_conflicts[i];

		//cout << "extra swapping conflict between vertices (" << v << "," << u << "), timestep " << t << " between " << a1 << " and " << a2 << endl;
		int a1_v_var = at[a1][v].first_variable + (t - at[a1][v].first_timestep);
		int a1_u_var = at[a1][u].first_variable + (t + 1 - at[a1][u].first_timestep);
		int a2_v_var = at[a2][v].first_variable + (t + 1 - at[a2][v].first_timestep);
		int a2_u_var = at[a2][u].first_variable + (t - at[a2][u].first_timestep);
		AddClause(vector<int> {-a1_v_var, -a1_u_var, -a2_v_var, -a2_u_var});
		nr_clauses_conflict++;
	}
}

void _MAPFSAT_ISolver::CreateConf_Swapping_Pass_OnDemand()
{
	for (size_t i = 0; i < swap_conflicts.size(); i++)
	{
		int a1, a2, v, u, t;
		tie(a1, a2, v, u, t) = swap_conflicts[i];

		int dir = -1, op_dir = -1;

		for (int d = 1; d < 5; d++)
		{
			if (inst->GetNeighbor(v, d) == u)
			{
				dir = d;
				op_dir = inst->OppositeDir(dir);
				break;
			}
		}

		//cout << "extra swapping conflict at edge (" << v << "," << u << "), timestep " << t << " between " << a1 << " and " << a2 << endl;
		int a1_var = pass[a1][v][dir].first_variable + (t - pass[a1][v][dir].first_timestep);
		int a2_var = pass[a2][u][op_dir].first_variable + (t - pass[a2][u][op_dir].first_timestep);
		AddClause(vector<int> {-a1_var, -a2_var});
		nr_clauses_conflict++;
	}	
}

void _MAPFSAT_ISolver::CreateConf_Swapping_Shift_OnDemand()
{
	for (size_t i = 0; i < swap_conflicts.size(); i++)
	{
		int a1, a2, v, u, t;
		tie(a1, a2, v, u, t) = swap_conflicts[i];

		int dir = -1, op_dir = -1;

		for (int d = 1; d < 5; d++)
		{
			if (inst->GetNeighbor(v, d) == u)
			{
				dir = d;
				op_dir = inst->OppositeDir(dir);
				break;
			}
		}
		
		size_t t_ind1 = find(shift[v][dir].timestep.begin(), shift[v][dir].timestep.end(), t) - shift[v][dir].timestep.begin();
		size_t t_ind2 = find(shift[u][op_dir].timestep.begin(), shift[u][op_dir].timestep.end(), t) - shift[u][op_dir].timestep.begin();
		
		//cout << "extra swapping conflict at edge (" << v << "," << u << "), timestep " << t << " using shift" << endl;
		int shift1_var = shift[v][dir].first_varaible + t_ind1;
		int shift2_var = shift[u][op_dir].first_varaible + t_ind2;
		AddClause(vector<int> {-shift1_var, -shift2_var});
		nr_clauses_conflict++;
	}
}

void _MAPFSAT_ISolver::CreateConf_Pebble_At_OnDemand()
{
	for (size_t i = 0; i < pebble_conflicts.size(); i++)
	{
		int a1, a2, u, v, t;
		tie(a1, a2, u, v, t) = pebble_conflicts[i];

		//cout << "extra pebble conflict at vertex " << v << ", timestep " << t << " between " << a1 << " and " << a2 << endl;
		int a1_var = at[a1][v].first_variable + (t - at[a1][v].first_timestep);
		int a2_var = at[a2][v].first_variable + (t - 1 - at[a2][v].first_timestep);
		AddClause(vector<int> {-a1_var, -a2_var});
		nr_clauses_conflict++;
	}
}

void _MAPFSAT_ISolver::CreateConf_Pebble_Pass_OnDemand()
{
	for (size_t i = 0; i < pebble_conflicts.size(); i++)
	{
		int a1, a2, v, u, t;
		tie(a1, a2, v, u, t) = pebble_conflicts[i];
		t--;

		int dir = -1;

		for (int d = 1; d < 5; d++)
		{
			if (inst->GetNeighbor(v, d) == u)
			{
				dir = d;
				break;
			}
		}

		//cout << "extra pebble conflict at edge (" << v << "," << u << "), timestep " << t << " between moving agent " << a1 << " and " << a2 << endl;
		int a1_var = pass[a1][v][dir].first_variable + (t - pass[a1][v][dir].first_timestep);
		int a2_var = at[a2][u].first_variable + (t - at[a2][u].first_timestep);
		AddClause(vector<int> {-a1_var, -a2_var});
		nr_clauses_conflict++;
	}
}

void _MAPFSAT_ISolver::CreateConf_Pebble_Shift_OnDemand()
{
	for (size_t i = 0; i < pebble_conflicts.size(); i++)
	{
		int a1, a2, v, u, t;
		tie(a1, a2, v, u, t) = pebble_conflicts[i];
		t--;

		int dir = -1;

		for (int d = 1; d < 5; d++)
		{
			if (inst->GetNeighbor(v, d) == u)
			{
				dir = d;
				break;
			}
		}

		size_t t_ind = find(shift[v][dir].timestep.begin(), shift[v][dir].timestep.end(), t) - shift[v][dir].timestep.begin();
		int shift1_var = shift[v][dir].first_varaible + t_ind;

		for (int u_dir = 1; u_dir < 5; u_dir++) // do not check waiting direction
		{
			size_t ind = find(shift[u][u_dir].timestep.begin(), shift[u][u_dir].timestep.end(), shift[v][dir].timestep[t_ind]) - shift[u][u_dir].timestep.begin();

			if (ind == shift[u][u_dir].timestep.size() || shift[u][u_dir].timestep[ind] != shift[v][dir].timestep[t_ind]) // did not find t in neighboring shift
				continue;
			
			//cout << "extra pebble conflict at edge (" << v << "," << u << "), timestep " << shift[v][dir].timestep[t_ind] << " direction " << u_dir << " using shift" << endl;
			int shift2_var = shift[u][u_dir].first_varaible + ind;
			AddClause(vector<int> {-shift1_var, -shift2_var});
			nr_clauses_conflict++;
		}
	}
}

void _MAPFSAT_ISolver::CreateConst_Avoid()
{
	for (size_t i = 0; i < inst->avoid_locations.size(); i++)
	{
		if (inst->avoid_locations[i].t < max_timestep)
		{
			int v = inst->map[inst->avoid_locations[i].v.y][inst->avoid_locations[i].v.x];
			int t = inst->avoid_locations[i].t;

			for (int a = 0; a < agents; a++)
			{
				if (at[a][v].first_variable == 0)
					continue;
				
				int at_var = at[a][v].first_variable + (t - at[a][v].first_timestep);
				AddClause(vector<int> {-at_var});
				nr_clauses_conflict++;
			}
		}
	}
}

/****************************/
// MARK: solver functions
/****************************/

int _MAPFSAT_ISolver::InvokeSolver(int timelimit)
{
	if (cnf_file.compare("") != 0)	// print cnf into file
	{
		std::ofstream out(cnf_file);
		if (out.is_open())
		{
			out << "p cnf " << nr_vars-1 << " " << nr_clauses << endl;
			out << cnf_printable.str() << endl;
			out.close();
		}
	}

	// save memory for SAT solver unless variables are still needed
	if (!print_plan && !keep_plan && lazy_const != 2)
	{
		CleanUp();
		cnf_printable.clear();
	}

	int res = -1;
	plan.clear();

	res = InvokeSolverImplementation(timelimit); // CaDiCaL or monosat

	// find conflicts if lazy encoding is used
	if (!plan.empty() && lazy_const == 2)
		GenerateConflicts();

	// print found plan
	if (!plan.empty() && print_plan && !conflicts_present)
		PrintPlan();

	solver_calls++;

	return res;
}

/****************************/
// MARK: plan output
/****************************/

int _MAPFSAT_ISolver::NormalizePlan()
{
	int max_t = 1;
	for (size_t a = 0; a < plan.size(); a++)
	{
		int goal = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];
		for (int t = (int)plan[a].size() - 1; t >= 0; t--)
		{
			if (plan[a][t] == -1)
				plan[a][t] = goal;
			if (plan[a][t] != goal)
			{
				max_t = max(max_t, (int)t + 2);
				break;
			}
		}
	}

	for (size_t a = 0; a < plan.size(); a++)
		plan[a].resize(max_t);

	return max_t;
}

void _MAPFSAT_ISolver::PrintPlan()
{
	int final_timesteps = 0;
	if (plan.size() > 0)
		final_timesteps = plan[0].size();

	VerifyPlan();

	cout << "Found plan [agents = " << agents << "] [timesteps = " << final_timesteps << "]" << endl;
	for (size_t a = 0; a < plan.size(); a++)
	{
		cout << "Agent #" << a << " : ";
		for (size_t t = 0; t < plan[a].size(); t++)
			cout << plan[a][t] << " ";
		cout << endl;
	}	
	cout << endl;	
}

void _MAPFSAT_ISolver::VerifyPlan()
{
	for (size_t a1 = 0; a1 < plan.size(); a1++)
	{
		for (size_t a2 = a1+1; a2 < plan.size(); a2++)
		{
			for (size_t t = 0; t < plan[a1].size(); t++)
			{
				if (plan[a1][t] == plan[a2][t])
					cout << "Vertex conflict! Agents " << a1 << ", " << a2 << ", timestep " << t << ", location " << plan[a1][t] << endl;
				
				if (t < plan[a1].size() - 1 && plan[a1][t] == plan[a2][t+1] && plan[a1][t+1] == plan[a2][t])
					cout << "Swapping conflict! Agents " << a1 << ", " << a2 << ", timestep " << t << ", edge (" << plan[a1][t] << "," << plan[a1][t+1] << ")" << endl;
				
				if (movement == 2 && t < plan[a1].size() - 1 && plan[a1][t] == plan[a2][t+1])
					cout << "Pebble conflict! Agent " << a2 << " moved into " << plan[a2][t+1] << " in " << t+1 << ", but " << a1 << " was present in previous timestep." << endl;
				
				if (movement == 2 && t < plan[a1].size() - 1 && plan[a1][t+1] == plan[a2][t])
					cout << "Pebble conflict! Agent " << a1 << " moved into " << plan[a1][t+1] << " in " << t+1 << ", but " << a2 << " was present in previous timestep." << endl;
			}
		}
	}
}

vector<vector<int> > _MAPFSAT_ISolver::GetPlan()
{
	return plan;
}

void _MAPFSAT_ISolver::GenerateConflicts()
{
	// add only new conflict, old conflict are already in the SAT solver
	vertex_conflicts.clear();
	swap_conflicts.clear();
	pebble_conflicts.clear();

	for (size_t a1 = 0; a1 < plan.size(); a1++)
	{
		for (size_t a2 = a1+1; a2 < plan.size(); a2++)
		{
			for (size_t t = 0; t < plan[a1].size(); t++)
			{
				if (plan[a1][t] == plan[a2][t])
				{
					conflicts_present = true;
					vertex_conflicts.push_back(make_tuple(a1,a2,plan[a1][t],t));
					//cout << "Vertex conflict! Agents " << a1 << ", " << a2 << ", timestep " << t << ", location " << plan[a1][t] << endl;
				}
				
				if (t < plan[a1].size() - 1 && plan[a1][t] == plan[a2][t+1] && plan[a1][t+1] == plan[a2][t] && plan[a1][t] != plan[a2][t])
				{
					conflicts_present = true;
					swap_conflicts.push_back(make_tuple(a1,a2,plan[a1][t],plan[a1][t+1],t));
					//cout << "Swapping conflict! Agents " << a1 << ", " << a2 << ", timestep " << t << ", edge (" << plan[a1][t] << "," << plan[a1][t+1] << ")" << endl;
				}

				if (movement == 2 && t < plan[a1].size() - 1 && plan[a1][t] == plan[a2][t+1] && plan[a2][t] != plan[a2][t+1])
				{
					conflicts_present = true;
					pebble_conflicts.push_back(make_tuple(a2,a1,plan[a2][t],plan[a2][t+1],t+1));
					//cout << "Pebble conflict! Agent " << a2 << " moved into " << plan[a2][t+1] << " in " << t+1 << ", but " << a1 << " was present in previous timestep." << endl;
				}

				if (movement == 2 && t < plan[a1].size() - 1 && plan[a1][t+1] == plan[a2][t] && plan[a1][t] != plan[a1][t+1])
				{
					conflicts_present = true;
					pebble_conflicts.push_back(make_tuple(a1,a2,plan[a1][t],plan[a1][t+1],t+1));
					//cout << "Pebble conflict! Agent " << a1 << " moved into " << plan[a1][t+1] << " in " << t+1 << ", but " << a2 << " was present in previous timestep." << endl;
				}
			}
		}
	}
}

/****************************/
// MARK: cleanup functions
/****************************/

bool _MAPFSAT_ISolver::TimesUp(	std::chrono::time_point<std::chrono::high_resolution_clock> start_time,
						std::chrono::time_point<std::chrono::high_resolution_clock> current_time,
						int timelimit) // timelimit is in ms
{
	if (chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count() > timelimit)
	{
		CleanUp();
		ReleaseSolver();
		return true;
	}
	return false;
}

void _MAPFSAT_ISolver::CleanUp()
{
	if (shift != NULL)
	{
		for (int v = 0; v < vertices; v++)
			delete[] shift[v];
		delete[] shift;
		shift = NULL;
	}

	if (shift_times_start != NULL)
	{
		delete[] shift_times_start;
		shift_times_start = NULL;
	}

	if (shift_times_end != NULL)
	{
		delete[] shift_times_end;
		shift_times_end = NULL;
	}

	if (pass != NULL)
	{
		for (int a = 0; a < agents; a++)
		{
			for (int v = 0; v < vertices; v++)
				delete[] pass[a][v];
			delete[] pass[a];
		}
		delete[] pass;
		pass = NULL;
	}

	if (at != NULL)
	{
		for (int a = 0; a < agents; a++)
			delete[] at[a];
		delete[] at;
		at = NULL;
	}
}