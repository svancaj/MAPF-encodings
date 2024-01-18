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

	_MAPFSAT_Instance* inst = new _MAPFSAT_Instance(map, start, goal);
	_MAPFSAT_Logger* log = new _MAPFSAT_Logger(inst, "encoding_name", 2);
	_MAPFSAT_PassParallelMksAll* solver = new _MAPFSAT_PassParallelMksAll("encoding_name", 1);

	solver->SetData(inst, log, 300, false, true);
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