#include "logger.hpp"

using namespace std;

_MAPFSAT_Logger::_MAPFSAT_Logger(_MAPFSAT_Instance* i, string enc, int type, string log_f)
{
	inst = i;
	log_file = log_f;
	map_name = inst->map_name;
	scen_name = inst->scen_name;
	encoding = enc;
	print_type = type;
}

void _MAPFSAT_Logger::PrintStatistics()
{
	if (print_type == 0) // no print
		return;

	string solution = "unsat";
	if (res == 0)
		solution = "sat";

	ostream* log;
	std::ofstream fout;
	if (log_file.empty())
	{
		log = &cout;
	}
	else
	{
		fout.open(log_file, ios::app);
		if (!fout.is_open())
		{
			cerr << "Could not open log file " << log_file << endl;
			return;
		}
		log = &fout;
	}
	
	if (print_type == 1)	// inline print
	{
		string sep = "\t";
		*log << map_name << sep <<
			scen_name << sep <<
			encoding << sep <<
			agents << sep <<
			mksLB << sep <<
			socLB << sep <<
			solution_mks << sep <<
			solution_soc << sep <<
			building_time << sep <<
			solving_time << sep <<
			solver_calls << sep <<
			nr_vars << sep <<
			nr_clauses << sep <<
			nr_clauses_move << sep <<
			nr_clauses_conflict << sep <<
			solution << sep <<
			endl;
	}

	if (print_type == 2)	// human readable print
	{ 
		string sep = "\n";
		*log << sep <<
			"========== Results ==========" << sep <<
			"Map name:           " << map_name << sep <<
			"Scenario name:      " << scen_name << sep <<
			"Encoding used:      " << encoding << sep <<
			"Number of agents:   " << agents << sep <<
			"Mks lower bound:    " << mksLB << sep <<
			"Soc lower bound:    " << socLB << sep <<
			"Solution mks:       " << solution_mks << sep <<
			"Solution soc:       " << solution_soc << sep <<
			"CNF building time:  " << building_time << sep <<
			"SAT solving time:   " << solving_time << sep <<
			"Nr of solver calls: " << solver_calls << sep <<
			"Nr of variables:    " << nr_vars << sep <<
			"Nr of clauses       " << nr_clauses << sep <<
			"Nr of move clauses: " << nr_clauses_move << sep <<
			"Nr of conf clauses: " << nr_clauses_conflict << sep <<
			"Found solution:     " << solution << sep <<
			endl << endl;
	}

	if (fout.is_open())
		fout.close();
}

void _MAPFSAT_Logger::NewInstance(int ags)
{
	agents = ags;
	mksLB = inst->GetMksLB(ags);
	socLB = inst->GetSocLB(ags);
	solution_mks = 0;
	solution_soc = 0;
	building_time = 0;
	solving_time = 0;
	nr_vars = 0;
	nr_clauses = 0;
	res = 1;
}