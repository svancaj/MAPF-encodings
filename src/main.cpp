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

	/****************************/
	// MARK: setting arguments
	/****************************/

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
	char *cvalue = NULL;

	int timeout = 300;
	string map_dir = "instances/maps";
	string stat_file = "";
	string cnf_file = "";
	int log_option = 0;

	_MAPFSAT_Instance* inst;
	_MAPFSAT_Logger* log;
	_MAPFSAT_ISolver* solver;

	// parse arguments
	opterr = 0;
	char c;
	while ((c = getopt (argc, argv, "hqpoe:s:m:a:i:t:d:f:l:c:")) != -1)
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
			case 'c':
				cvalue = optarg;
				break;
			case '?':
				if (optopt == 'e' || optopt == 's' || optopt == 'm' || optopt == 'a' || optopt == 'i' || optopt == 't' || optopt == 'd' || optopt == 'f' || optopt == 'l' || optopt == 'c')
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

	/****************************/
	// MARK: check arguments
	/****************************/

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
		cerr << "Unknown encoding \"" << evalue << "\"!" << endl;
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

	if (cvalue != NULL)
		cnf_file = cvalue;

	// create classes and load map
	inst = new _MAPFSAT_Instance(map_dir, svalue);
	log = new _MAPFSAT_Logger(inst, evalue, log_option, stat_file);
	solver->SetData(inst, log, timeout, cnf_file, qflag, pflag);

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

	/****************************/
	// MARK: solving instances
	/****************************/

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
			log->PrintStatistics();
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
	cout << "*******************************************************" << endl;
	cout << "*        This is a reduction-based MAPF solver        *" << endl;
	cout << "*          Created by Jiri Svancara @ MFF UK          *" << endl;
	cout << "*       Used SAT solvers are CaDiCaL and Monosat      *" << endl;
	cout << "*******************************************************" << endl;
	cout << endl;
}

/****************************/
// MARK: help
/****************************/

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
	cout << "	-e encoding         : Encoding to be used. Available options are {mks|soc}_{parallel|pebble}_{at|pass|shift|monosat-pass|monosat-shift}_{eager|lazy}_{single|dupli}" << endl;
	cout << "	-s scenario_file    : Path to a scenario file" << endl;
	cout << "	-m map_dir          : Directory containing map files. Default is instances/maps" << endl;
	cout << "	-a number_of_agents : Number of agents to solve. If not specified, all agents in the scenario file are used." << endl;
	cout << "	-i increment        : After a successful call, increase the number of agents by the specified increment. If not specified, do not perform subsequent calls." << endl;
	cout << "	-t timeout          : Timeout of the computation in seconds. Default value is 300s" << endl;
	cout << "	-d delta            : Cost of delta is added to the first call. Default is 0." << endl;
	cout << "	-o                  : Oneshot solving. Ie. do not increment cost in case of unsat call. Default is to optimize." << endl;
	cout << "	-f log_file         : log file. If not specified, output to stdout." << endl;
	cout << "	-l log_level        : logging option. 0 = no log, 1 = inline log, 2 = human readable log. if -f log_file is not specified in combination with -l 1 or -l 2 overrides -q. Default is 0." << endl;
	cout << "	-c cnf_file         : print the created CNF into cnf_file. If not specified, the created CNF is not printed." << endl;
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

/****************************/
// MARK: selecting encoding
/****************************/

_MAPFSAT_ISolver* PickEncoding(string enc)
{
	_MAPFSAT_ISolver* solver = NULL;

	int cost = -1, moves = -1, var = -1, lazy = -1, dupli = -1, satsolver = 1;

	stringstream ssline(enc);
	string part;
	vector<string> parsed_options;
	while (getline(ssline, part, '_'))
		parsed_options.push_back(part);

	if (parsed_options.size() != 5)
		return NULL;

	// cost function
	if (parsed_options[0].compare("mks") == 0)
		cost = 1;
	if (parsed_options[0].compare("soc") == 0)
		cost = 2;
	if (cost < 0)
		return NULL;

	// motion
	if (parsed_options[1].compare("parallel") == 0)
		moves = 1;
	if (parsed_options[1].compare("pebble") == 0)
		moves = 2;
	if (moves < 0)
		return NULL;

	// variables
	if (parsed_options[2].compare("at") == 0)
		var = 1;
	if (parsed_options[2].compare("pass") == 0)
		var = 2;
	if (parsed_options[2].compare("shift") == 0)
		var = 3;
	if (parsed_options[2].compare("monosat-pass") == 0)
	{
		var = 2;
		satsolver = 2;
	}
	if (parsed_options[2].compare("monosat-shift") == 0)
	{
		var = 3;
		satsolver = 2;
	}
	if (var < 0)
		return NULL;

	// lazy
	if (parsed_options[3].compare("eager") == 0)
		lazy = 1;
	if (parsed_options[3].compare("lazy") == 0)
		lazy = 2;
	if (lazy < 0)
		return NULL;

	// duplicate agents
	if (parsed_options[4].compare("single") == 0)
		dupli = 1;
	if (parsed_options[4].compare("dupli") == 0)
		dupli = 2;
	if (dupli < 0)
		return NULL;

	if (satsolver == 1) 
		solver = new _MAPFSAT_SAT(var,cost,moves,lazy,dupli,satsolver,enc);
	if (satsolver == 2) 
		solver = new _MAPFSAT_SMT(var,cost,moves,lazy,dupli,satsolver,enc);

	return solver;
}