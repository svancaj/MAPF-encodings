#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <queue>
#include <algorithm>

struct Vertex
{
	int x;
	int y;

	bool operator==(const Vertex &rhs) const
	{
		if (rhs.x == x && rhs.y == y)
			return true;
		return false;
    }
};

struct Agent
{
	Vertex start;
	Vertex goal;
};

class Instance
{
public:
	Instance(std::string, std::string);

	void SetAgents(int);
	
	int GetMksLB(int);
	int GetSocLB(int);

	void DebugPrint(std::vector<std::vector<int> >&);
	void DebugPrint(std::vector<int>&);
	void DebugPrint(std::vector<Vertex>&);

	std::vector<Agent> agents;
	std::vector<std::vector<int> > length_from_start;
	std::vector<std::vector<int> > length_from_goal;
	std::vector<int> SP_lengths;

	std::vector<std::vector<int> > map;
	size_t height;
	size_t width;
	size_t number_of_vertices;

	std::string scen_name;
	std::string map_name;

private:
	void LoadAgents(std::string, std::string);
	void LoadMap(std::string);
	void BFS(std::vector<int>&, Vertex);
	
	std::vector<int> mks_LBs;
	std::vector<int> soc_LBs;

	int last_number_of_agents;
};
