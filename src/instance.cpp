#include "instance.hpp"

using namespace std;

/* Constructor */

Instance::Instance(string map_dir, string agents_file)
{
	LoadAgents(agents_file, map_dir);
	last_number_of_agents = 0;
	scen_name = agents_file;
}


/* Public functions */

int Instance::GetMksLB(size_t ags)
{
	if (mks_LBs[ags] >= 0)
		return mks_LBs[ags];
	int LB = 0;
	for (size_t i = 0; i < ags; i++)
		LB = max(LB, SP_lengths[i]);
	mks_LBs[ags] = LB;
	return LB;
}

int Instance::GetSocLB(size_t ags)
{
	if (soc_LBs[ags] >= 0)
		return soc_LBs[ags];
	int LB = 0;
	for (size_t i = 0; i < ags; i++)
		LB += SP_lengths[i];
	soc_LBs[ags] = LB;
	return LB;
}

void Instance::SetAgents(int ags)
{
	for (int i = last_number_of_agents; i < ags; i++)
	{
		length_from_start[i] = vector<int>(number_of_vertices, -1);
		length_from_goal[i] = vector<int>(number_of_vertices, -1);

		BFS(length_from_start[i], agents[i].start);
		BFS(length_from_goal[i], agents[i].goal);

		SP_lengths[i] = length_from_start[i][map[agents[i].goal.x][agents[i].goal.y]];

		mks_LBs[i+1] = max(mks_LBs[i], SP_lengths[i] + 1);
		soc_LBs[i+1] = soc_LBs[i] + SP_lengths[i] + 1;
	}
	last_number_of_agents = ags;
}

Vertex Instance::IDtoCoords(int vertex)
{
	return coord_list[vertex];
}

bool Instance::HasNeighbor(Vertex v, int dir)
{
	if (dir == 0) // self loop
		return true;

	if (dir == 1 && v.x > 0 && map[v.x - 1][v.y] != -1)
		return true;
	if (dir == 2 && v.x < height - 1 && map[v.x + 1][v.y] != -1)
		return true;
	if (dir == 3 && v.y > 0 && map[v.x][v.y - 1] != -1 )
		return true;
	if (dir == 4 && v.y < width - 1 && map[v.x][v.y + 1] != -1 )
		return true;
	return false;
}

bool Instance::HasNeighbor(int v, int dir)
{
	if (dir == 0) // self loop
		return true;
	return HasNeighbor(IDtoCoords(v), dir);
}

int Instance::GetNeighbor(int ver, int dir) // assumes the neighbor exists
{
	if (dir == 0) // self loop
		return ver;

	Vertex v = IDtoCoords(ver);

	if (dir == 1 && v.x > 0)
		return map[v.x - 1][v.y];
	if (dir == 2 && v.x < height - 1)
		return map[v.x + 1][v.y];
	if (dir == 3 && v.y > 0)
		return map[v.x][v.y - 1];
	if (dir == 4 && v.y < width - 1)
		return map[v.x][v.y + 1];
	return -1;
}

int Instance::FirstTimestep(int agent, int vertex)
{
	return length_from_start[agent][vertex];
}

int Instance::LastTimestepMks(int agent, int vertex, int timelimit)
{
	return timelimit - length_from_goal[agent][vertex] - 1;
}

bool Instance::IsReachableMks(int agent, int vertex, int timestep, int timelimit)
{
	if (FirstTimestep(agent, vertex) <= timestep && LastTimestepMks(agent, vertex, timelimit) >= timestep)
		return true;
	return false;
}

int Instance::OppositeDir(int dir)
{
	if (dir == 1)
		return 2;
	if (dir == 2)
		return 1;
	if (dir == 3)
		return 4;
	if (dir == 4)
		return 3;
	return -1;
}


/* Private functions */

void Instance::LoadAgents(string agents_path, string map_dir)
{
	// Read input
	bool map_loaded = false;
	ifstream in;
	in.open(agents_path);
	if (!in.is_open())
	{
		cout << "Could not open " << agents_path << endl;
		return;
	}

	string line;
	getline(in, line); // first line - version

	while (getline(in, line))
	{
		stringstream ssline(line);
		string part;
		vector<string> parsed_line;
		while (getline(ssline, part, '\t'))
			parsed_line.push_back(part);

		if (!map_loaded)
		{
			map_name = parsed_line[1];
			LoadMap(map_dir.append("/").append(map_name));
			map_loaded = true;
		}

		Agent new_agent;
		new_agent.start = {size_t(stoi(parsed_line[5])), size_t(stoi(parsed_line[4]))};
		new_agent.goal = {size_t(stoi(parsed_line[7])), size_t(stoi(parsed_line[6]))};

		agents.push_back(new_agent);
	}

	mks_LBs = vector<int>(agents.size() + 1, -1);
	soc_LBs = vector<int>(agents.size() + 1, -1);
	mks_LBs[0] = 0;
	soc_LBs[0] = 0;
	SP_lengths = vector<int>(agents.size(), 0);
	length_from_start = vector<vector<int> >(agents.size());
	length_from_goal = vector<vector<int> >(agents.size());

	in.close();
}

void Instance::LoadMap(string map_path)
{
	ifstream in;
	in.open(map_path);
	if (!in.is_open())
	{
		cout << "Could not open " << map_path << endl;
		return;
	}

	char c_dump;
	string s_dump;
	getline(in, s_dump); // first line - type

	in >> s_dump >> height;
	in >> s_dump >> width;
	in >> s_dump; // map
	
	// graph
	map = vector<vector<int> >(height, vector<int>(width, -1));
	number_of_vertices = 0;

	for (size_t i = 0; i < height; i++)
	{
		for (size_t j = 0; j < width; j++)
		{
			in >> c_dump;
			if (c_dump == '.')
			{
				map[i][j] = number_of_vertices;
				number_of_vertices++;
				coord_list.push_back({i,j});
			}
		}
	}

	in.close();
}

void Instance::BFS(vector<int>& length_from, Vertex start)
{
	queue<Vertex> que;

	length_from[map[start.x][start.y]] = 0;
	que.push(start);

	while(!que.empty())
	{
		Vertex v = que.front();
		que.pop();

		if (HasNeighbor(v,1) && length_from[map[v.x - 1][v.y]] == -1)
		{
			length_from[map[v.x - 1][v.y]] = length_from[map[v.x][v.y]] + 1;
			que.push({v.x - 1, v.y});
		}
		if (HasNeighbor(v,2) && length_from[map[v.x + 1][v.y]] == -1)
		{
			length_from[map[v.x + 1][v.y]] = length_from[map[v.x][v.y]] + 1;
			que.push({v.x + 1, v.y});
		}
		if (HasNeighbor(v,3) && length_from[map[v.x][v.y - 1]] == -1)
		{
			length_from[map[v.x][v.y - 1]] = length_from[map[v.x][v.y]] + 1;
			que.push({v.x, v.y - 1});
		}
		if (HasNeighbor(v,4) && length_from[map[v.x][v.y + 1]] == -1)
		{
			length_from[map[v.x][v.y + 1]] = length_from[map[v.x][v.y]] + 1;
			que.push({v.x, v.y + 1});
		}
	}
}


/* DEBUG */

void Instance::DebugPrint(vector<vector<int> >& map_to_print)
{
	for (size_t i = 0; i < map_to_print.size(); i++)
	{
		for (size_t j = 0; j < map_to_print[i].size(); j++)
			if (map[i][j] == -1)
				cout << "@";
			else if (map_to_print[i][j] == -1)
				cout << "#";
			else
				cout << ".";
		cout << endl;
	}
}

void Instance::DebugPrint(vector<int>& vc_to_print)
{
	for (size_t i = 0; i < vc_to_print.size(); i++)
		cout << vc_to_print[i] << " ";
	cout << endl;
}

void Instance::DebugPrint(vector<Vertex>& vc_to_print)
{
	for (size_t i = 0; i < vc_to_print.size(); i++)
		cout << "(" << vc_to_print[i].x << ", " << vc_to_print[i].y << ") ";
	cout << endl;
}