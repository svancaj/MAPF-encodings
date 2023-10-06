#include <stdlib.h>
#include <unistd.h>

#include "instance.hpp"
#include "logger.hpp"
#include "encodings/solver_common.hpp"
#include "encodings/pass_parallel_mks_all.hpp"
#include "encodings/pass_parallel_soc_all.hpp"

using namespace std;

void PrintIntro();
void PrintHelp(char**);
void CleanUp(Instance*, Logger*, ISolver*);
ISolver* PickEncoding(string);

int main(int argc, char** argv) 
{
	PrintIntro();

	bool hflag = false;
	char *evalue = NULL;
	char *svalue = NULL;
	char *avalue = NULL;
	char *ivalue = NULL;
	char *tvalue = NULL;

	int timeout = 300;
	string map_dir = "instances/maps";
	string stat_file = "results.res";

	Instance* inst;
	Logger* log;
	ISolver* solver;

	// parse arguments
	opterr = 0;
	char c;
	while ((c = getopt (argc, argv, "he:s:a:i:t:")) != -1)
	{
		switch (c)
		{
			case 'h':
				hflag = true;
				break;
			case 'e':
				evalue = optarg;
				break;
			case 's':
				svalue = optarg;
				break;
			case 'a':
				avalue = optarg;
				break;
			case 'i':
				ivalue = optarg;
				break;
			case 't':
				tvalue = optarg;
				break;
			case '?':
				if (optopt == 'e' || optopt == 's' || optopt == 'a' || optopt == 'i' || optopt == 't')
				{
					cout << "Option -" << (char)optopt << " requires an argument!" << endl;
					return -1;
				}
				// unknown option - ignore it
				break;
			default:
				return -1; // should not get here;
		}
	}

	// check pased arguments
	if (hflag)
	{
		PrintHelp(argv);
		return 0;
	}

	if (evalue == NULL || svalue == NULL)
	{
		cout << "Missing a required argument!" << endl;
		PrintHelp(argv);
		return -1;
	}

	if ((solver = PickEncoding(string(evalue))) == NULL)
	{
		cout << "Unknown base algorithm \"" << evalue << "\"!" << endl;
		PrintHelp(argv);
		return -1;
	}
	//solver = new Pass_parallel_mks_all(inst, log, string(evalue));

	if (tvalue != NULL)
		timeout = atoi(tvalue);

	// create classes and load map
	inst = new Instance(map_dir, svalue);
	log = new Logger(inst, stat_file, evalue);
	solver->SetData(inst, log, timeout);

	// check number of agents and increment
	size_t current_agents = inst->agents.size();
	if (avalue != NULL)
		current_agents = size_t(stoi(avalue));
	if (current_agents > inst->agents.size())
	{
		cout << "Invalid number of agents. There are " << inst->agents.size() << " in the " << svalue << " scenario file." << endl;
		CleanUp(inst, log, solver);
		return -1;
	}
	
	size_t increment = inst->agents.size();
	if (ivalue != NULL)
		increment = atoi(ivalue);
	if (increment == 0)
		increment = inst->agents.size();

	// main loop - solve given number of agents
	do 
	{
		inst->SetAgents(current_agents);
		log->NewInstance(current_agents);

		int res = solver->Solve(current_agents);

		if (res == 1) // timeout
		{
			cout << "No solution found in the given timeout" << endl;
			break;
		}
		
		log->PrintStatistics();
		current_agents += increment;
	}
	while (current_agents <= inst->agents.size());

	//inst->DebugPrint(inst->map);

	CleanUp(inst, log, solver);
	return 0;
}

void PrintIntro()
{
	cout << endl;
	cout << "*****************************************" << endl;
	cout << "* This is a reduction-based MAPF solver *" << endl;
	cout << "*   Created by Jiri Svancara @ MFF UK   *" << endl;
	cout << "*       Used SAT solver is Kissat       *" << endl;
	cout << "*****************************************" << endl;
	cout << endl;
}

void PrintHelp(char* argv[])
{
	cout << endl;
	cout << "Usage of this program:" << endl;
	cout << argv[0] << " [-h] -e encoding -s scenario_file [-a number_of_agents] [-i increment] [-t timeout]" << endl;
	cout << "	-h                  : Prints help and exits" << endl;
	cout << "	-e encoding         : Encoding to be used. Available options are {at|pass|shift}_{pebble|parallel}_{mks|soc}_{all|jit}" << endl;
	cout << "	-s scenario_file    : Path to a scenario file" << endl;
	cout << "	-a number_of_agents : Number of agents to solve. If not specified, all agents in the scenario file are used." << endl;
	cout << "	-i increment        : After a successful call, increase the number of agents by the specified increment. If not specified, do not perform subsequent calls." << endl;
	cout << "	-t timeout          : Timeout of the computation in seconds. Default value is 300s" << endl;
	cout << endl;
}

void CleanUp(Instance* inst, Logger* log, ISolver* solver)
{
	if (inst != NULL)
		delete inst;
	if (log != NULL)
		delete log;
	if (solver != NULL)
		delete solver;
}

ISolver* PickEncoding(string enc)
{
	ISolver* solver = NULL;
	if (enc.compare("pass_parallel_mks_all") == 0)
	{
		solver = new Pass_parallel_mks_all(enc, 1);
		return solver;
	}
	if (enc.compare("pass_parallel_soc_all") == 0)
	{
		solver = new Pass_parallel_soc_all(enc, 2);
		return solver;
	}
	return solver;
}