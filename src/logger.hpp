#ifndef _logger_h_INCLUDED
#define _logger_h_INCLUDED

#include "instance.hpp"

class _MAPFSAT_Logger
{

public:
	_MAPFSAT_Logger(_MAPFSAT_Instance*, std::string, int = 0, std::string = "");

	void PrintStatistics(bool = true);
	void NewInstance(int);

	std::string map_name;
	std::string scen_name;
	std::string encoding;
	int solution_mks;
	int solution_soc;
	int building_time;
	int solving_time;
	int nr_vars;
	int nr_clauses;
	int solver_calls;
	int agents;
	int mksLB;
	int socLB;	
	
private:
	_MAPFSAT_Instance* inst;
	std::string log_file;

	int print_type;

};

#endif
