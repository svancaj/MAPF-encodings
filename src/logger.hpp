#pragma once

#include "instance.hpp"

class Logger
{

public:
	Logger(Instance*, std::string, std::string);

	void PrintStatistics(std::vector<std::vector<int> >&);
	void NewInstance(int);
	int GetMks(std::vector<std::vector<int> >&);
	int GetSoc(std::vector<std::vector<int> >&);

private:
	Instance* inst;
	std::string log_file;

	std::string map_name;
	std::string scen_name;
	std::string encoding;
	int agents;
	int mksLB;
	int socLB;
	int solution_mks;
	int solution_soc;
	int building_time;
	int solving_time;
	int nr_vars;
	int nr_clauses;
};