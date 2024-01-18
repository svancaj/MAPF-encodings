#ifndef _instance_h_INCLUDED
#define _instance_h_INCLUDED

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <queue>
#include <algorithm>

struct _MAPFSAT_Vertex
{
	size_t x;
	size_t y;

	bool operator==(const _MAPFSAT_Vertex &rhs) const
	{
		if (rhs.x == x && rhs.y == y)
			return true;
		return false;
    }
};

struct _MAPFSAT_Agent
{
	_MAPFSAT_Vertex start;
	_MAPFSAT_Vertex goal;
};

class _MAPFSAT_Instance
{
public:
	/** Constructor of _MAPFSAT_Instance using files.
    *
    * Stores all relevant data about the solved instance.
    *
    * @param map_dir directory including the map.
    * @param agents_file file containing the solved scenario.
    */
    _MAPFSAT_Instance(std::string, std::string);

    /** Constructor of _MAPFSAT_Instance using data.
    *
    * Stores all relevant data about the solved instance.
    *
    * @param map_vector the map, obstacles are marked by -1
    * @param starts agents start locations - x, y coords
    * @param goals agents goal locations - x, y coords
    * @param scenstr name of the scenario, for logging purposes, default is "scen"
    * @param mapstr name of the map, for logging purposes, default is "map"
    */
    _MAPFSAT_Instance(std::vector<std::vector<int> >&, std::vector<std::pair<int,int> >&, std::vector<std::pair<int,int> >&, std::string = "scen", std::string = "map");

    /** Set the number of agents to be computed.
    *
    * Computes reachability and lower bounds for agents that were not used before. Does not check if ags is out of bounds.
    *
    * @param ags number of agents.
    */
    void SetAgents(int);
	
	int GetMksLB(size_t);
	int GetSocLB(size_t);

	_MAPFSAT_Vertex IDtoCoords(int);
	bool HasNeighbor(_MAPFSAT_Vertex, int);
	bool HasNeighbor(int, int);
	int GetNeighbor(int, int);
	
	int FirstTimestep(int, int);
	int LastTimestep(int, int, int, int, int);
	int OppositeDir(int);

	void DebugPrint(std::vector<std::vector<int> >&);
	void DebugPrint(std::vector<int>&);
	void DebugPrint(std::vector<_MAPFSAT_Vertex>&);

	std::vector<_MAPFSAT_Agent> agents;
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
	void LoadAgentsData(std::vector<std::pair<int,int> >&, std::vector<std::pair<int,int> >&);
	void LoadMap(std::string);
	void LoadMapData(std::vector<std::vector<int> >&);
	void BFS(std::vector<int>&, _MAPFSAT_Vertex);
	
	std::vector<int> mks_LBs;
	std::vector<int> soc_LBs;
	std::vector<_MAPFSAT_Vertex> coord_list;

	size_t last_number_of_agents;
};

#endif
