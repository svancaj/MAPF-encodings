#include "solver_common.hpp"

using namespace std;

int _MAPFSAT_ShiftPebbleSocAll::CreateFormula(vector<vector<int> >& CNF, int time_left)
{
	int timesteps = inst->GetMksLB(agents) + delta;

	int lit = 1;

	auto start = chrono::high_resolution_clock::now();

	// create variables
	lit = CreateAt(lit, timesteps);
	lit = CreateShift(lit, timesteps);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// start - goal possitions
	CreatePossition_Start(CNF);
	CreatePossition_Goal(CNF);
	CreatePossition_NoneAtGoal(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// conflicts
	CreateConf_Vertex(CNF);
	//CreateConf_Swapping_Shift(CNF);
	CreateConf_Pebble_Shift(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;
	
	// movement 
	CreateMove_NoDuplicates(CNF);
	CreateMove_NextVertex_At(CNF);
	CreateMove_ExactlyOne_Shift(CNF);
	CreateMove_NextVertex_Shift(CNF);
	if (TimesUp(start, chrono::high_resolution_clock::now(), time_left))
		return -1;

	// soc limit
	if (delta > 0)
		lit = CreateConst_LimitSoc(CNF, lit);

	// Deallocate memory
	CleanUp(print_plan);

	return lit;
}
