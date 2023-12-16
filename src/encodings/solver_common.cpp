#include "solver_common.hpp"

using namespace std;

/****************************/
/****** before solving ******/
/****************************/

void ISolver::SetData(Instance* i, Logger* l, int to, bool q, bool p)
{
	inst = i;
	log = l;
	timeout = to; // in s
	quiet = q;
	print_plan = p;

	if (quiet)
		print_plan = false;

	at = NULL;
	pass = NULL;
	shift = NULL;
};

void ISolver::PrintSolveDetails()
{
	if (quiet)
		return;
	
	cout << "Currently solving" << endl;
	cout << "Instance name: " << inst->scen_name << endl;
	cout << "Map name: " << inst->map_name << endl;
	cout << "Encoding used: " << solver_name << endl;
	cout << "Optimizing function: " << ((cost_function == 1) ? "makespan" : "sum of costs") << endl;
	cout << "Number of agents: " << agents << endl;
	cout << "Mks LB: " << inst->GetMksLB(agents) << endl;
	cout << "SoC LB: " << inst->GetSocLB(agents) << endl;
	cout << "Delta: " << delta << endl;
	cout << endl;
};

void ISolver::DebugPrint(vector<vector<int> >& CNF)
{
	for (size_t i = 0; i < CNF.size(); i++)
	{
		for (size_t j = 0; j < CNF[i].size(); j++)
		{
			cout << CNF[i][j] << " ";
		}
		cout << "0\n";
	}
	cout << "0" << endl;
}

/********************************/
/********* SOLVING MAPF *********/
/********************************/

int ISolver::Solve(int ags, int input_delta, bool oneshot)
{
	delta = input_delta;
	int time_left = timeout * 1000; // given in s, tranfer to ms
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
		CNF = vector<vector<int> >();

		// create formula
		auto start = chrono::high_resolution_clock::now();
		nr_vars = CreateFormula(CNF, time_left);
		auto stop = chrono::high_resolution_clock::now();
		if (TimesUp(start, stop, time_left))
			return 1;

		nr_clauses = CNF.size();
		building_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= building_time;

		// solve formula
		start = chrono::high_resolution_clock::now();
		res = InvokeSolver(CNF, time_left + 100);
		stop = chrono::high_resolution_clock::now();
		if (TimesUp(start, stop, time_left))
			return 1;

		solving_time += chrono::duration_cast<chrono::milliseconds>(stop - start).count();
		time_left -= solving_time;

		if (res == 0) // ok
		{
			log->nr_vars = nr_vars;
			log->nr_clauses = nr_clauses;
			log->building_time = building_time;
			log->solving_time = solving_time;
			log->solution_mks = inst->GetMksLB(agents) + delta;
			log->solution_soc = (cost_function == 1) ? 0 : inst->GetSocLB(agents) + delta;
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

/****************************/
/***** creating formula *****/
/****************************/

int ISolver::CreateAt(int lit, int timesteps)
{
	at = new TEGAgent*[agents];

	for (int a = 0; a < agents; a++)
	{
		at[a] = new TEGAgent[vertices];
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
	at_vars = lit; // at vars alwayes start at 1
	return lit;
}

int ISolver::CreatePass(int lit, int timesteps)
{
	pass = new TEGAgent**[agents];

	for (int a = 0; a < agents; a++)
	{
		pass[a] = new TEGAgent*[vertices];
		for (int v = 0; v < vertices; v++)
		{
			pass[a][v] = new TEGAgent[5]; // 5 directions from a vertex
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

void ISolver::CreatePossition_Start(std::vector<std::vector<int> >& CNF)
{
	for (int a = 0; a < agents; a++)
	{
		int start_var = at[a][inst->map[inst->agents[a].start.x][inst->agents[a].start.y]].first_variable;
		CNF.push_back(vector<int> {start_var});
	}
}

void ISolver::CreatePossition_Goal(std::vector<std::vector<int> >& CNF)
{
	for (int a = 0; a < agents; a++)
	{
		TEGAgent AV_goal = at[a][inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y]];
		int goal_var = AV_goal.first_variable + (AV_goal.last_timestep - AV_goal.first_timestep);
		CNF.push_back(vector<int> {goal_var});
	}
}

void ISolver::CreatePossition_NoneAtGoal(std::vector<std::vector<int> >& CNF)
{
	for (int a = 0; a < agents; a++)
	{
		int star_t = at[a][inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y]].last_timestep + 1;
		int v = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];
		for (int a2 = a+1; a2 < agents; a2++)
		{
			if (at[a2][v].first_variable == 0)
				continue;
			int end_t = at[a2][v].last_timestep;
			for (int t = star_t; t < end_t; t++)
			{
				//cout << a2 << " can not be at " << v << ", timestep " << t << " becuase " << a << " is in goal there" << endl;
				int a2_var = at[a2][v].first_variable + (t - at[a2][v].first_timestep);
				CNF.push_back(vector<int> {-a2_var});
			}
		}
	}
}

void ISolver::CreateConf_Vertex(std::vector<std::vector<int> >& CNF)
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
					CNF.push_back(vector<int> {-a1_var, -a2_var});
				}
			}
		}
	}
}

void ISolver::CreateConf_Swapping_At(std::vector<std::vector<int> >& CNF)
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
						CNF.push_back(vector<int> {-a1_v_var, -a1_u_var, -a2_v_var, -a2_u_var});
					}
				}
			}
		}
	}
}

void ISolver::CreateConf_Swapping_Pass(std::vector<std::vector<int> >& CNF)
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
						CNF.push_back(vector<int> {-a1_var, -a2_var});
					}
				}
			}
		}
	}
}

void ISolver::CreateConf_Pebble_At(std::vector<std::vector<int> >& CNF)
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
					CNF.push_back(vector<int> {-a1_var, -a2_var});
				}

			}

		}
	}
}

void ISolver::CreateConf_Pebble_Pass(std::vector<std::vector<int> >& CNF)
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
						CNF.push_back(vector<int> {-a1_var, -a2_var});
					}
				}
			}
		}
	}
}

void ISolver::CreateMove_NoDuplicates(std::vector<std::vector<int> >& CNF)
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
					CNF.push_back(vector<int> {-v_var, -u_var});
				}
			}
		}
	}
}

void ISolver::CreateMove_NextVertex_At(std::vector<std::vector<int> >& CNF)
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
				
				int at_var = at[a][v].first_variable + (t - at[a][v].first_timestep);;
				neibs.push_back(-at_var);

				CNF.push_back(neibs);
			}
		}
	}
}

void ISolver::CreateMove_EnterVertex_Pass(std::vector<std::vector<int> >& CNF)
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
					CNF.push_back(vector<int> {-pass_var, at_var});
				}
			}
		}
	}
}

void ISolver::CreateMove_LeaveVertex_Pass(std::vector<std::vector<int> >& CNF)
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
				
				int at_var = at[a][v].first_variable + (t - at[a][v].first_timestep);;
				neibs.push_back(-at_var);

				CNF.push_back(neibs);
			}
		}
	}
}

int ISolver::CreateConst_LimitSoc(vector<vector<int>>& CNF, int lit)
{
	vector<int> late_variables;

	for (int a = 0; a < agents; a++)
	{
		int goal_v = inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y];
		int at_var = at[a][goal_v].first_variable;
		for (int d = 0; d < delta; d++)
		{
			CNF.push_back(vector<int> {at_var + d, lit});	// if agent is not at goal, it is late
			if (d < delta - 1)
				CNF.push_back(vector<int> {lit, -(lit + 1)});	// if agent is late at t, it is late at t+1
			late_variables.push_back(lit);
			lit++;
		}
	}

	// add constraint on sum of delays
	PB2CNF pb2cnf;
	vector<vector<int> > formula;
	lit = pb2cnf.encodeAtMostK(late_variables, delta, formula, lit) + 1;

	for (size_t i = 0; i < formula.size(); i++)
		CNF.push_back(formula[i]);

	return lit;
}

/*******************************/
/********* solving CNF *********/
/*******************************/

int ISolver::InvokeSolver(vector<vector<int>> &CNF, int timelimit)
{
	kissat* solver = kissat_init();
    kissat_set_option(solver, "quiet", 1);
	
	// CNF to solver
	for (size_t i = 0; i < CNF.size(); i++)
	{
		for (size_t j = 0; j < CNF[i].size(); j++)
			kissat_add(solver, CNF[i][j]);
		kissat_add(solver, 0);
	}

	CleanUp(print_plan);	// save memory for kissat
	CNF = vector<vector<int> >();

	bool ended = false;
	thread waiting_thread = thread(wait_for_terminate, timelimit, solver, ref(ended));
	
    int ret = kissat_solve(solver); // Start solver

	ended = true;
	solver_calls++;

	if (print_plan && ret == 10)	// variable assignment
	{
		vector<bool> eval = vector<bool>(at_vars);
		for (int var = 1; var < at_vars; var++)
			eval[var-1] = (kissat_value (solver, var) > 0) ? true : false;

		vector<vector<int> > plan = vector<vector<int> >(agents, vector<int>(max_timestep));

		for (int a = 0; a < agents; a++)
		{
			for (int v = 0; v < vertices; v++)
			{
				if (at[a][v].first_variable == 0)
					continue;

				for (int t = at[a][v].first_timestep; t < at[a][v].last_timestep + 1; t++)
				{
					int var = at[a][v].first_variable + (t - at[a][v].first_timestep);
					if (eval[var-1])
						plan[a][t] = v;
				}
			}
		}

		cout << "Found plan [agents = " << agents << "] [timesteps = " << max_timestep << "]" << endl;
		for (int a = 0; a < agents; a++)
		{
			cout << "Agent #" << a << " : ";
			for (int t = 0; t < max_timestep; t++)
				cout << plan[a][t] << " ";
			cout << endl;
		}		
	}

	CleanUp(false);
	kissat_release(solver);
	waiting_thread.join();

	return (ret == 10) ? 0 : 1;
}

void ISolver::wait_for_terminate(int time_left_ms, kissat* solver, bool& ended)
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
		
	kissat_terminate(solver);	// Trusting in kissat implementation
}

bool ISolver::TimesUp(	std::chrono::time_point<std::chrono::high_resolution_clock> start_time,
						std::chrono::time_point<std::chrono::high_resolution_clock> current_time,
						int timelimit) // timelimit is in ms
{
	if (chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count() > timelimit)
	{
		CleanUp(false);
		return true;
	}
	return false;
}

void ISolver::CleanUp(bool keep_at)
{
	if (shift != NULL)
	{
		// TODO
		shift = NULL;
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

	if (!keep_at && at != NULL)
	{
		for (int a = 0; a < agents; a++)
			delete[] at[a];
		delete[] at;
		at = NULL;
	}
}