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

	Instance* inst = new Instance(map, start, goal, "scenario", "map_name");
	Logger* log = new Logger(inst, "log.log", "encoding_name");
	Pass_parallel_mks_all* solver = new Pass_parallel_mks_all("encoding_name", 1);

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

	return 0;
}