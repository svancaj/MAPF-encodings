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
	_MAPFSAT_Logger* log = new _MAPFSAT_Logger(inst, "encoding_name", 0);
	_MAPFSAT_DisappearAtGoal* solver = new _MAPFSAT_DisappearAtGoal();

	solver->SetData(inst, log, 300, "", true, true);
	inst->SetAgents(2);
	log->NewInstance(2);

	//int res = solver->Solve(2);
	int res = solver->Solve(2, 2, false, true);

	if (res == -1)
		cout << "no solution with given limits" << endl;
	if (res == 0)
		cout << "found solution" << endl;
	if (res == 1)
		cout << "no solution in the given timelimit" << endl;

	auto sol = solver->GetPlan();

	cout << "Plan:" << endl;
	for (size_t i = 0; i < sol.size(); i++)
	{
		for (size_t j = 0; j < sol[i].size(); j++)
		{
			cout << sol[i][j] << " ";
		}
		cout << endl;
	}

	log->PrintStatistics();

	//inst->DebugPrint(inst->map);

	delete inst;
	delete log;
	delete solver;

	return 0;
}