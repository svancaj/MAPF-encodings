#include <stdlib.h>
#include <unistd.h>

#include "instance.hpp"
#include "logger.hpp"
#include "encodings/isolver.hpp"
#include "encodings/pass_parallel_mks_all.hpp"

using namespace std;

void PrintHelp(char**);
void CleanUp(Instance*, Logger*, ISolver*);
bool CheckEncoding(ISolver*, Instance*, Logger*, string);

int main(int argc, char** argv) 
{
	bool hflag = false;
	char *evalue = NULL;
	char *svalue = NULL;
	char *avalue = NULL;
	char *ivalue = NULL;
	char *tvalue = NULL;

	srand('a');

	int timeout = 300;
	string map_dir = "instances/maps";
	string stat_file = "results.dat";

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

	if (!CheckEncoding(solver, inst, log, string(evalue)))
	{
		cout << "Unknown base algorithm \"" << evalue << "\"!" << endl;
		PrintHelp(argv);
		return -1;
	}

	if (tvalue != NULL)
		timeout = atoi(tvalue);

	// create classes and load map
	inst = new Instance(map_dir, svalue);
	log = new Logger(inst, stat_file, evalue);

	// check number of agents and increment
	size_t current_agents = inst->agents.size();
	if (avalue != NULL)
		current_agents = stoi(avalue);
	if (current_agents > inst->agents.size() || current_agents < 0)
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
	
	cout << "total agents " << inst->agents.size() << endl;
	// main loop - solve given number of agents
	do 
	{
		cout << "in main - solving " << current_agents << " agents" << endl;
		inst->SetAgents(current_agents);
		log->NewInstance(current_agents);

		int res = solver->Solve(current_agents);

		if (res == 1) // timeout
		{
			cout << "No solution found in the give timeout" << endl;
			break;
		}
		
		vector<vector<int> > plan = vector<vector<int> >();
		log->PrintStatistics(plan);
		current_agents += increment;
	}
	while (current_agents <= inst->agents.size());

	//inst->DebugPrint(inst->map);

	cout << "cleanup start" << endl;
	CleanUp(inst, log, solver);
	cout << "cleanup end" << endl;

	return 0;
}

void PrintHelp(char* argv[])
{
	cout << "Usage of this program:" << endl;
	cout << argv[0] << " [-h] -e encoding -s scenario_file [-a number_of_agents] [-i increment] [-t timeout]" << endl;
	cout << "	-h                  : Prints help and exits" << endl;
	cout << "	-e encoding         : Encoding to be used. Available options are {at|pass|shift}_{pebble|parallel}_{mks|soc}_{all|jit}" << endl;
	cout << "	-s scenario_file    : Path to a scenario file" << endl;
	cout << "	-a number_of_agents : Number of agents to solve. If not specified, all agents in the scenario file are used." << endl;
	cout << "	-i increment        : After a successful call, increase the number of agents by the specified increment. If not specified, do not perform subsequent calls." << endl;
	cout << "	-t timeout          : Timeout of the computation. Default value is 300s" << endl;
}

void CleanUp(Instance* inst, Logger* log, ISolver* solver)
{
	if (inst != NULL)
	{
		delete inst;
		cout << "clean1" << endl;
	}
	if (log != NULL)
	{
		delete log;
		cout << "clean2" << endl;
	}
	if (solver != NULL)
	{
		delete solver;
		cout << "clean3" << endl;
	}
}

bool CheckEncoding(ISolver* solver, Instance* inst, Logger* log, string enc)
{
	if (enc.compare("pass_parallel_mks_all") == 0)
	{
		cout << "creating solver" << endl;
		solver = new Pass_parallel_mks_all(inst, log, enc);
		return true;
	}
	return false;
}