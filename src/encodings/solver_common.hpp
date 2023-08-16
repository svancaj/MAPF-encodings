#pragma once

#include <unistd.h>
#include <chrono>

#include "../instance.hpp"
#include "../logger.hpp"

struct TEGAgent
{
	int first_variable;
	int first_timestep;
	int last_timestep;
};

class ISolver
{
public:
	ISolver(std::string sn, int opt) : solver_name(sn), cost_function(opt) {}; 
	virtual ~ISolver() {};
	
	virtual int Solve(int) = 0;
	void SetData(Instance*, Logger*, int);

protected:
	Instance* inst;
	Logger* log;
	std::string solver_name;
	int cost_function; // 1 = mks, 2 = soc
	int timeout;

	int agents;
	int vertices;
	int delta;
	TEGAgent** at;
	TEGAgent*** pass;
	TEGAgent*** shift;
	std::vector<std::vector<int> > CNF;
	int nr_vars;
	int nr_clauses;
	int solver_calls;

	void PrintSolveDetails();
	void DebugPrint(std::vector<std::vector<int> >& );

	// creating formula
	int CreateAt(int, int);
	int CreatePass(int, int);

	void CreatePossition_Start(std::vector<std::vector<int> >&);
	void CreatePossition_Goal(std::vector<std::vector<int> >&);
	void CreatePossition_NoneAtGoal(std::vector<std::vector<int> >&);
	void CreateConf_Vertex(std::vector<std::vector<int> >&);
	void CreateConf_Swapping_Pass(std::vector<std::vector<int> >&);
	void CreateMove_NoDuplicates(std::vector<std::vector<int> >&);
	void CreateMove_EnterVertex_Pass(std::vector<std::vector<int> >&);
	void CreateMove_LeaveVertex_Pass(std::vector<std::vector<int> >&);


	// solving
	void PrintCNF(std::vector<std::vector<int> >&);
	int InvokeSolver(int, bool);
	bool TimesUp(std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>, int);
	void CleanUp();
};

