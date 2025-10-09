#include <stdlib.h>
#include <unistd.h>

#include "MAPF.hpp"

using namespace std;

int main() 
{
	vector<vector<int> > map = 
		{
			{-1,-1,1,-1},
			{1,1,1,1}
		};
	vector<pair<int,int> > start = {{1,0},{1,1}};
	vector<pair<int,int> > goal = {{1,1},{1,0}};

	int cost = 2;				// 1 = mks, 		2 = soc
	int moves = 1;				// 1 = parallel, 	2 = pebble
	int var = 1;				// 1 = at, 			2 = pass, 		3 = shift
	int lazy = 1;				// 1 = eager, 		2 = lazy
	int dupli = 1;				// 1 = forbid, 		2 = allow
	int satsolver = 1;			// 1 = CaDiCaL,		2 = monosat (use _MAPFSAT_SMT for monosat)
	string enc = "soc_parallel_at_eager_single";

	_MAPFSAT_Instance* inst = new _MAPFSAT_Instance(map, start, goal);
	_MAPFSAT_Logger* log = new _MAPFSAT_Logger(inst, enc, 2);
	_MAPFSAT_SAT* solver = new _MAPFSAT_SAT(var,cost,moves,lazy,dupli,satsolver,enc);

	solver->SetData(inst, log, 300, "", false, true);
	inst->SetAgents(2);
	log->NewInstance(2);

	//int res = solver->Solve(2);
	int res = solver->Solve(2, 2, false);

	if (res == -1)
		cout << "no solution with given limits" << endl;
	if (res == 0)
		cout << "found solution" << endl;
	if (res == 1)
		cout << "no solution in the given timelimit" << endl;

	log->PrintStatistics();

	//inst->DebugPrint(inst->map);

	delete inst;
	delete log;
	delete solver;

	return 0;
}