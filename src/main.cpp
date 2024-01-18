#include <stdlib.h>
#include <unistd.h>

#include "instance.hpp"
#include "logger.hpp"
#include "encodings/solver_common.hpp"

using namespace std;

void PrintIntro(bool);
void PrintHelp(char**, bool);
void CleanUp(_MAPFSAT_Instance*, _MAPFSAT_Logger*, _MAPFSAT_ISolver*);
_MAPFSAT_ISolver* PickEncoding(string);

int main(int argc, char** argv) 
{
	bool hflag = false;
	bool qflag = false;
	bool pflag = false;
	bool oflag = false;
	char *evalue = NULL;
	char *svalue = NULL;
	char *mvalue = NULL;
	char *avalue = NULL;
	char *ivalue = NULL;
	char *tvalue = NULL;
	char *dvalue = NULL;
	char *fvalue = NULL;
	char *lvalue = NULL;

	int timeout = 300;
	string map_dir = "instances/maps";
	string stat_file = "";
	int log_option = 0;

	_MAPFSAT_Instance* inst;
	_MAPFSAT_Logger* log;
	_MAPFSAT_ISolver* solver;

	// parse arguments
	opterr = 0;
	char c;
	while ((c = getopt (argc, argv, "hqpoe:s:m:a:i:t:d:f:l:")) != -1)
	{
		switch (c)
		{
			case 'h':
				hflag = true;
				break;
			case 'q':
				qflag = true;
				break;
			case 'p':
				pflag = true;
				break;
			case 'o':
				oflag = true;
				break;
			case 'e':
				evalue = optarg;
				break;
			case 's':
				svalue = optarg;
				break;
			case 'm':
				mvalue = optarg;
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
			case 'd':
				dvalue = optarg;
				break;
			case 'f':
				fvalue = optarg;
				break;
			case 'l':
				lvalue = optarg;
				break;
			case '?':
				if (optopt == 'e' || optopt == 's' || optopt == 'm' || optopt == 'a' || optopt == 'i' || optopt == 't' || optopt == 'd' || optopt == 'f' || optopt == 'l')
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

	PrintIntro(qflag);

	// check pased arguments
	if (hflag)
	{
		PrintHelp(argv, false);
		return 0;
	}

	if (evalue == NULL || svalue == NULL)
	{
		cerr << "Missing a required argument!" << endl;
		PrintHelp(argv, qflag);
		return -1;
	}

	if ((solver = PickEncoding(string(evalue))) == NULL)
	{
		cerr << "Unknown base algorithm \"" << evalue << "\"!" << endl;
		PrintHelp(argv, qflag);
		return -1;
	}

	if (mvalue != NULL)
		map_dir = mvalue;

	if (tvalue != NULL)
		timeout = atoi(tvalue);

	if (fvalue != NULL)
		stat_file = fvalue;

	if (lvalue != NULL)
		log_option = atoi(lvalue);
	if (log_option != 0 && log_option != 1 && log_option != 2)
	{
		cerr << "Invalid log level!" << endl;
		PrintHelp(argv, qflag);
		return -1;
	}

	// create classes and load map
	inst = new _MAPFSAT_Instance(map_dir, svalue);
	log = new _MAPFSAT_Logger(inst, evalue, log_option, stat_file);
	solver->SetData(inst, log, timeout, qflag, pflag);

	// check number of agents and increment
	size_t current_agents = inst->agents.size();
	if (avalue != NULL)
		current_agents = size_t(stoi(avalue));
	if (current_agents > inst->agents.size())
	{
		cerr << "Invalid number of agents. There are " << inst->agents.size() << " in the " << svalue << " scenario file." << endl;
		CleanUp(inst, log, solver);
		return -1;
	}
	
	size_t increment = inst->agents.size();
	if (ivalue != NULL)
		increment = atoi(ivalue);
	if (increment == 0)
		increment = inst->agents.size();
	
	int delta = 0;
	if (dvalue != NULL)
		delta = atoi(dvalue);

	// main loop - solve given number of agents
	do 
	{
		inst->SetAgents(current_agents);
		log->NewInstance(current_agents);

		int res = solver->Solve(current_agents, delta, oflag);

		if (res == 1) // timeout
		{
			if (!qflag)
				cout << "No solution found in the given timeout" << endl;
			break;
		}

		if (res == -1) // unsat with given cost
		{
			log->PrintStatistics(false);
			if (!qflag)
				cout << "No solution found in the given cost limit" << endl;
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

void PrintIntro(bool quiet)
{
	if (quiet)
		return;

	cout << endl;
	cout << "*****************************************" << endl;
	cout << "* This is a reduction-based MAPF solver *" << endl;
	cout << "*   Created by Jiri Svancara @ MFF UK   *" << endl;
	cout << "*       Used SAT solver is Kissat       *" << endl;
	cout << "*****************************************" << endl;
	cout << endl;
}

void PrintHelp(char* argv[], bool quiet)
{
	if (quiet)
		return;

	cout << endl;
	cout << "Usage of this program:" << endl;
	cout << argv[0] << " [-h] [-q] [-p] -e encoding -s scenario_file [-m map_dir] [-a number_of_agents] [-i increment] [-t timeout] [-d delta] [-o] [-f log_file]" << endl;
	cout << "	-h                  : Prints help and exits" << endl;
	cout << "	-q                  : Suppress print on stdout" << endl;
	cout << "	-p                  : Print found plan. If q flag is set, p flag is overwritten." << endl;
	cout << "	-e encoding         : Encoding to be used. Available options are {at|pass|shift}_{pebble|parallel}_{mks|soc}_{all|jit}" << endl;
	cout << "	-s scenario_file    : Path to a scenario file" << endl;
	cout << "	-m map_dir          : Directory containing map files. Default is instances/maps" << endl;
	cout << "	-a number_of_agents : Number of agents to solve. If not specified, all agents in the scenario file are used." << endl;
	cout << "	-i increment        : After a successful call, increase the number of agents by the specified increment. If not specified, do not perform subsequent calls." << endl;
	cout << "	-t timeout          : Timeout of the computation in seconds. Default value is 300s" << endl;
	cout << "	-d delta            : Cost of delta is added to the first call. Default is 0." << endl;
	cout << "	-o                  : Oneshot solving. Ie. do not increment cost in case of unsat call. Default is to optimize." << endl;
	cout << "	-f log_file         : log file. If not specified, output to stdout." << endl;
	cout << "	-l log_level        : logging option. 0 = no log, 1 = inline log, 2 = human readable log. if -f log_file is not specified in combination with -l 1 or -l 2 overrides -q. Default is 0." << endl;
	cout << endl;
}

void CleanUp(_MAPFSAT_Instance* inst, _MAPFSAT_Logger* log, _MAPFSAT_ISolver* solver)
{
	if (inst != NULL)
		delete inst;
	if (log != NULL)
		delete log;
	if (solver != NULL)
		delete solver;
}

_MAPFSAT_ISolver* PickEncoding(string enc)
{
	_MAPFSAT_ISolver* solver = NULL;
	if (enc.compare("at_parallel_mks_all") == 0)
	{
		solver = new _MAPFSAT_AtParallelMksAll(enc, 1);
		return solver;
	}
	if (enc.compare("at_parallel_soc_all") == 0)
	{
		solver = new _MAPFSAT_AtParallelSocAll(enc, 2);
		return solver;
	}
	if (enc.compare("at_pebble_mks_all") == 0)
	{
		solver = new _MAPFSAT_AtPebbleMksAll(enc, 1);
		return solver;
	}
	if (enc.compare("at_pebble_soc_all") == 0)
	{
		solver = new _MAPFSAT_AtPebbleSocAll(enc, 2);
		return solver;
	}
	if (enc.compare("pass_parallel_mks_all") == 0)
	{
		solver = new _MAPFSAT_PassParallelMksAll(enc, 1);
		return solver;
	}
	if (enc.compare("pass_parallel_soc_all") == 0)
	{
		solver = new _MAPFSAT_PassParallelSocAll(enc, 2);
		return solver;
	}
	if (enc.compare("pass_pebble_mks_all") == 0)
	{
		solver = new _MAPFSAT_PassPebbleMksAll(enc, 1);
		return solver;
	}
	if (enc.compare("pass_pebble_soc_all") == 0)
	{
		solver = new _MAPFSAT_PassPebbleSocAll(enc, 2);
		return solver;
	}
	return solver;
}