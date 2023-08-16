#include "logger.hpp"

using namespace std;

Logger::Logger(Instance* i, string log_f, string enc)
{
	inst = i;
	log_file = log_f;
	map_name = inst->map_name;
	scen_name = inst->scen_name;
	encoding = enc;
}

void Logger::PrintStatistics()
{
	ofstream log;
	log.open(log_file, ios::app);
	if (log.is_open())
	{
		string sep = "\t";
		log << map_name << sep <<
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
			nr_clauses << endl;
	}
	else
	{
		cout << "Could not open log file!" << endl;
		return;
	}

	log.close();
}

void Logger::NewInstance(int ags)
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
}