#include "pass_parallel_mks_all.hpp"

using namespace std;

int Pass_parallel_mks_all::Solve(int agents)
{
	int delta = 10;

	while (true)
	{
		int res = 0; // 0 = ok, 1 = timeout
		PrintSolveDetails(agents, delta);

		// create formula
		auto start = chrono::high_resolution_clock::now();
		CNF = vector<vector<int> >();
		nr_vars = CreateFormula(CNF, agents, delta);
		nr_clauses = CNF.size();
		auto stop = chrono::high_resolution_clock::now();
		auto building_time = chrono::duration_cast<chrono::milliseconds>(stop - start).count();

		// solve formula
		PrintCNF(CNF);
		start = chrono::high_resolution_clock::now();
		res = InvokeSolver();
		stop = chrono::high_resolution_clock::now();
		auto solving_time = chrono::duration_cast<chrono::milliseconds>(stop - start).count();

		if (res == 0) // ok
		{
			log->nr_vars = nr_vars;
			log->nr_clauses = nr_clauses;
			log->building_time = building_time;
			log->solving_time = solving_time;
			break;
		}
		if (res == 1) // timeout
			return 1;
		delta++; // no solution with given limits, increase delta
	}

	return 0;
}

int Pass_parallel_mks_all::CreateFormula(vector<vector<int> >& CNF, int agents, int delta)
{
	int timesteps = inst->GetMksLB(agents) + delta;
	int vertices = inst->number_of_vertices;

	TEGAgent** at = new TEGAgent*[agents];
	TEGAgent*** pass = new TEGAgent**[agents];

	int lit = 1;

	// create "at" variables
	for (int a = 0; a < agents; a++)
	{
		at[a] = new TEGAgent[vertices];
		for (int v = 0; v < vertices; v++)
		{
			if (inst->FirstTimestep(a, v) <= inst->LastTimestepMks(a, v, timesteps))
			{
				at[a][v].first_variable = lit;
				at[a][v].first_timestep = inst->FirstTimestep(a, v);
				at[a][v].last_timestep = inst->LastTimestepMks(a, v, timesteps);
				lit += at[a][v].last_timestep - at[a][v].first_timestep + 1;
			}
			else
			{
				at[a][v].first_variable = 0;
			}
		}
	}

	// create "pass" variables
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
				if (inst->FirstTimestep(a, v) < inst->LastTimestepMks(a, inst->GetNeighbor(v, dir), timesteps)) // !!! might be wrong
				{
					pass[a][v][dir].first_variable = lit;
					pass[a][v][dir].first_timestep = inst->FirstTimestep(a, v);
					pass[a][v][dir].last_timestep = inst->LastTimestepMks(a, inst->GetNeighbor(v, dir), timesteps) - 1;
					lit += pass[a][v][dir].last_timestep - pass[a][v][dir].first_timestep + 1;
				}
				else
				{
					pass[a][v][dir].first_variable = 0;
				}
			}
		}
	}

	// start - goal possitions
	for (int a = 0; a < agents; a++)
	{
		int start_var = at[a][inst->map[inst->agents[a].start.x][inst->agents[a].start.y]].first_variable;

		TEGAgent AV_goal = at[a][inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y]];
		int goal_var = AV_goal.first_variable + (AV_goal.last_timestep - AV_goal.first_timestep);

		CNF.push_back(vector<int> {start_var});
		CNF.push_back(vector<int> {goal_var});
	}

	// vertex conflict
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

	// swapping conflict
	for (int v = 0; v < vertices; v++)
	{
		for (int dir = 0; dir < 5; dir++)
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

	// movement - agent can't be at two diff vertices at same time
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
	
	// movement - moving over edge brings the agent to a vertex
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

	// movement - pick one of the outgoing edges
	for (int a = 0; a < agents; a++)
	{
		for (int v = 0; v < vertices; v++)
		{
			if (at[a][v].first_variable == 0)
				continue;

			int star_t = at[a][v].first_timestep;
			int end_t =at[a][v].last_timestep + 1;

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

	//DebugPrint(CNF);

	// Deallocate memory
	
	for (int a = 0; a < agents; a++)
	{
		delete[] at[a];
		delete[] pass[a];
	}
	delete[] at;
	delete[] pass;

	return lit;
}

void Pass_parallel_mks_all::PrintCNF(vector<vector<int> >& CNF)
{
	ofstream dimacs;
	dimacs.open("input.cnf");
	if (dimacs.is_open())
	{
		dimacs << "p cnf " << nr_vars << " " << nr_clauses << endl;
		for (size_t i = 0; i < CNF.size(); i++)
		{
			for (size_t j = 0; j < CNF[i].size(); j++)
			{
				dimacs << CNF[i][j] << " ";
			}
			dimacs << "0\n";
		}
	}
	else
		return;
	dimacs.close();
}

int Pass_parallel_mks_all::InvokeSolver()
{
	int res = system("./build/kissat-3.1.0-linux-amd64 input.cnf -q -n --time=1");
	cout << res << endl;
	return 0;
}
