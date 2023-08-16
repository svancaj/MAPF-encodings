#include "solver_common.hpp"

using namespace std;

/****************************/
/****** before solving ******/
/****************************/

void ISolver::SetData(Instance* i, Logger* l, int to)
	{
		inst = i;
		log = l;
		timeout = to; // in s
	};

void ISolver::PrintSolveDetails(int delta)
{
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

	return lit;
}

void ISolver::CreatePossition_StartGoal(std::vector<std::vector<int> >& CNF)
{
	for (int a = 0; a < agents; a++)
	{
		int start_var = at[a][inst->map[inst->agents[a].start.x][inst->agents[a].start.y]].first_variable;

		TEGAgent AV_goal = at[a][inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y]];
		int goal_var = AV_goal.first_variable + (AV_goal.last_timestep - AV_goal.first_timestep);

		CNF.push_back(vector<int> {start_var});
		CNF.push_back(vector<int> {goal_var});
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

void ISolver::CreateConf_Swapping_Pass(std::vector<std::vector<int> >& CNF)
{
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
}

/***************************/
/********* solving *********/
/***************************/

void ISolver::PrintCNF(vector<vector<int>> &CNF)
{
	ofstream dimacs;
	dimacs.open("run/input.cnf");
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

int ISolver::InvokeSolver(int timelimit, bool get_soltuion)
{
	string output_file = "run/result.out";
	string n = "-n";
	if (get_soltuion)
		n = "";

	string call = "./build/kissat-3.1.0-linux-amd64 run/input.cnf -q " + n + " --time=" + to_string(timelimit) + " > " + output_file;
	system(call.c_str());
	solver_calls++;

	ifstream res;
	res.open(output_file);
	if (!res.is_open())
	{
		cout << "Could not open solution file " << output_file << endl;
		return -1;
	}

	bool sat = false;
	string line;

	while (getline(res, line))
	{
		if (line.rfind("s", 0) == 0)	// solution
		{
			stringstream ssline(line);
			string part;
			vector<string> parsed_line;
			while (getline(ssline, part, ' '))
				parsed_line.push_back(part);
			if (parsed_line[1].compare("SATISFIABLE") == 0)
				sat = true;	
		}
		if (get_soltuion && line.rfind("v", 0) == 0)	// variable assignment
		{
			// TODO
			cout << line << endl;
		}
	}

	return (sat) ? 0 : 1;
}

bool ISolver::TimesUp(	std::chrono::time_point<std::chrono::high_resolution_clock> start_time,
						std::chrono::time_point<std::chrono::high_resolution_clock> current_time,
						int timelimit) // timelimit is in s
{
	if (chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count() > timelimit*1000)
	{
		CleanUp();
		return true;
	}
	return false;
}

void ISolver::CleanUp()
{
	for (int a = 0; a < agents; a++)
	{
		delete[] at[a];
		delete[] pass[a];
	}
	delete[] at;
	delete[] pass;
}