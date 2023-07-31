#include "pass_parallel_mks_all.hpp"

using namespace std;

int Pass_parallel_mks_all::Solve(int agents)
{
	int delta = 0;

	while (true)
	{
		PrintSolveDetails(agents, delta);

		// create formula
		auto start = chrono::high_resolution_clock::now();
		CNF = vector<vector<int> >();
		int nr_vars = CreateFormula(CNF, agents, delta);
		int nr_clauses = CNF.size();
		auto stop = chrono::high_resolution_clock::now();
		auto building_time = chrono::duration_cast<chrono::milliseconds>(stop - start).count();

		// solve formula
		int res = 0; // 0 = ok, 1 = timeout

		if (res == 0) // ok
		{
			log->nr_vars = nr_vars;
			log->nr_clauses = nr_clauses;
			log->building_time = building_time;
			log->solving_time = 111;
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

	int*** at = new int**[timesteps];
	int*** pass = new int**[timesteps - 1];

	int lit = 1;

	// create "at" variables
	for (int t = 0; t < timesteps; t++)
	{
		at[t] = new int*[agents];
		for (int a = 0; a < agents; a++)
		{
			at[t][a] = new int[vertices];
			for (int v = 0; v < vertices; v++)
			{
				if (inst->IsReachableMks(a, v, t, timesteps))
				{
					at[t][a][v] = lit;
					lit++;
				}
				else
				{
					at[t][a][v] = 0;
				}
			}
		}
	}

	// create "pass" variables
	for (int t = 0; t < timesteps - 1; t++)
	{
		pass[t] = new int*[agents];
		for (int a = 0; a < agents; a++)
		{
			pass[t][a] = new int[vertices*5];	// works only for 4-connected grid maps!!!
			for (int v = 0; v < vertices; v++)
			{
				if (!inst->IsReachableMks(a, v, t, timesteps))
					continue;
				for (int dir = 0; dir < 5; dir++)
				{
					if (!inst->HasNeighbor(v, dir))
						continue;
					if (inst->IsReachableMks(a, inst->GetNeighbor(v, dir), t+1, timesteps))
					{
						pass[t][a][v*5 + dir] = lit;
						lit++;
					}
					else
					{
						pass[t][a][v*5 + dir] = 0;
					}
				}
			}
		}
	}

	for (int a = 0; a < agents; a++)
	{
		CNF.push_back(vector<int> {at[0][a][inst->map[inst->agents[a].start.x][inst->agents[a].start.y]]});
		CNF.push_back(vector<int> {at[timesteps - 1][a][inst->map[inst->agents[a].goal.x][inst->agents[a].goal.y]]});
	}

	for (int a1 = 0; a1 < agents; a1++)
	{
		for (int a2 = a1 + 1; a2 < agents; a2++)
		{
			for (int v = 0; v < vertices; v++)
			{
				int star_t = max(inst->FirstTimestep(a1, v), inst->FirstTimestep(a2,v) - 1);
				int end_t = min(inst->LastTimestepMks(a1,v,timesteps), inst->LastTimestepMks(a2,v,timesteps) + 1) + 1;
				for (int t = star_t; t < end_t; t++)
				{
					if (at[t][a1][v] == 0)
						continue;

					if (at[t][a2][v] != 0)
						// no two agents can be at same vertex at same time
						CNF.push_back(vector<int> {-at[t][a1][v], -at[t][a2][v]});

					if (t == timesteps - 1)
						continue;
					for (int dir = 1; dir < 5; dir++)
					{
						int u = inst->GetNeighbor(v, dir);
						if (u > v && pass[t][a1][v*5 + dir] != 0 && pass[t][a2][u*5 + inst->OppositeDir(dir)] != 0)
							// no swapping: no two agents can move along same edge in opposite directions at same time
							CNF.push_back(vector<int> {-pass[t][a1][v*5 + dir], -pass[t][a2][u*5 + inst->OppositeDir(dir)]});
					}
				}
			}
		}
	}
	
	for (int a = 0; a < agents; a++)
	{
		for (int v = 0; v < vertices; v++)
		{
			for (int t = inst->FirstTimestep(a, v); t < inst->LastTimestepMks(a, v, timesteps) + 1; t++)
			{
				if (at[t][a][v] == 0 || t == timesteps - 1)
					continue;

				for (int dir = 0; dir < 5; dir++)
				{
					int u = inst->GetNeighbor(v, dir);
					if (pass[t][a][v*5 + dir] != 0)
						// if an agent is moving on an edge, it arrives in the next timestep
						CNF.push_back(vector<int> {-pass[t][a][v*5 + dir], at[t+1][a][u]});
				}

				for (int u = v+1; u < vertices; u++)
					if (at[t][a][u] != 0)
						// agent can't be at two diff vertices at same time
						CNF.push_back(vector<int> {-at[t][a][u], -at[t][a][v]});
				
				// if an agent is in a vertex, it moves out by one of the outgoing edges 
				vector<int> neibs;
				for (int dir = 0; dir < 5; dir++)
					if (pass[t][a][v*5 + dir] != 0)
						neibs.push_back(pass[t][a][v*5 + dir]);
				if (neibs.empty())
					continue;
				neibs.push_back(-at[t][a][v]);
				CNF.push_back(neibs);
			}
		}
	}

	// Deallocate memory
	for (int t = 0; t < timesteps; t++)
	{
		for (int a = 0; a < agents; a++)
			delete[] at[t][a];
		delete[] at[t];
	}

	for (int t = 0; t < timesteps - 1; t++)
	{
		for (int a = 0; a < agents; a++)
			delete[] pass[t][a];
		delete[] pass[t];
	}

	delete[] at;
	delete[] pass;

	return lit;
}