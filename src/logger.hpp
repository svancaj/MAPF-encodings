#ifndef _logger_h_INCLUDED
#define _logger_h_INCLUDED

#include "instance.hpp"

class _MAPFSAT_Logger
{

public:
	/** Constructor of _MAPFSAT_Logger.
    *
    * Stores all relevant data about the solution.
    *
    * @param instance pointer to a _MAPFSAT_Instance.
    * @param enc name of the encoding used by the soler.
    * @param log_level logging details - 0 = no log, 1 = inline log, 2 = human readable log. Default is 0.
    * @param log_file file output of the log. If "", print to stdout. Default is "".
    */
    _MAPFSAT_Logger(_MAPFSAT_Instance*, std::string, int = 0, std::string = "");

    /** Resets all currently stored data and prepares for the next solver call.
    *
    * @param ags number of agents to be solved in the next solver call.
    */
    void NewInstance(int);

    /** Prints stored statistics.
    *
    * Print is formatted as specified in the constructor. If there is no solution due to timeout, the log may not be correct.
    *
    */
    void PrintStatistics();

	std::string map_name;
	std::string scen_name;
	std::string encoding;
	int solution_mks;
	int solution_soc;
	int building_time;
	int solving_time;
	int nr_vars;
	int nr_clauses;
    int nr_clauses_move;
    int nr_clauses_conflict;
    int nr_clauses_soc;
    int nr_clauses_assumption;
	int solver_calls;
	int agents;
	int mksLB;
	int socLB;	
	int res;
	
private:
	_MAPFSAT_Instance* inst;
	std::string log_file;

	int print_type;

};

#endif
