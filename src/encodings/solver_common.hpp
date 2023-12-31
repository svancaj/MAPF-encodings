#ifndef _solver_common_h_INCLUDED
#define _solver_common_h_INCLUDED

#include <unistd.h>
#include <chrono>
#include <thread>

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
	
	int Solve(int, int = 0, bool = false);
	void SetData(Instance*, Logger*, int, bool, bool);

protected:
	Instance* inst;
	Logger* log;
	std::string solver_name;
	int cost_function; // 1 = mks, 2 = soc
	int timeout;
	bool quiet;
	bool print_plan;

	int agents;
	int vertices;
	int delta;
	int max_timestep;

	TEGAgent** at;
	TEGAgent*** pass;
	TEGAgent*** shift;
	int at_vars;

	std::vector<std::vector<int> > CNF;

	int nr_vars;
	int nr_clauses;
	int solver_calls;

	// before solving
	void PrintSolveDetails();
	void DebugPrint(std::vector<std::vector<int> >& );

	// virtual encoding to be used
	virtual int CreateFormula(std::vector<std::vector<int> >&, int) = 0;

	// creating formula
	int CreateAt(int, int);
	int CreatePass(int, int);

	void CreatePossition_Start(std::vector<std::vector<int> >&);
	void CreatePossition_Goal(std::vector<std::vector<int> >&);
	void CreatePossition_NoneAtGoal(std::vector<std::vector<int> >&);

	void CreateConf_Vertex(std::vector<std::vector<int> >&);
	void CreateConf_Swapping_At(std::vector<std::vector<int> >&);
	void CreateConf_Swapping_Pass(std::vector<std::vector<int> >&);
	void CreateConf_Pebble_At(std::vector<std::vector<int> >&);
	void CreateConf_Pebble_Pass(std::vector<std::vector<int> >&);

	void CreateMove_NoDuplicates(std::vector<std::vector<int> >&);
	void CreateMove_NextVertex_At(std::vector<std::vector<int> >&);
	void CreateMove_EnterVertex_Pass(std::vector<std::vector<int> >&);
	void CreateMove_LeaveVertex_Pass(std::vector<std::vector<int> >&);

	int CreateConst_LimitSoc(std::vector<std::vector<int> >&, int);


	// solving
	int InvokeSolver(std::vector<std::vector<int> >&, int);
	static void wait_for_terminate(int, void*, bool&);
	bool TimesUp(std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>, int);
	void CleanUp(bool);
};

/******************************************************************/
/*********************** Specific Encodings ***********************/
/******************************************************************/

class At_parallel_mks_all : public ISolver
{
public:
	using ISolver::ISolver;
	~At_parallel_mks_all() {};

private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class At_parallel_soc_all : public ISolver
{
public:
	using ISolver::ISolver;
	~At_parallel_soc_all() {};

private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class At_pebble_mks_all : public ISolver
{
public:
	using ISolver::ISolver;
	~At_pebble_mks_all() {};

private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class At_pebble_soc_all : public ISolver
{
public:
	using ISolver::ISolver;
	~At_pebble_soc_all() {};

private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class Pass_parallel_mks_all : public ISolver
{
public:
	using ISolver::ISolver;
	~Pass_parallel_mks_all() {};

private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class Pass_parallel_soc_all : public ISolver
{
public:
	using ISolver::ISolver;
	~Pass_parallel_soc_all() {};

private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class Pass_pebble_mks_all : public ISolver
{
public:
	using ISolver::ISolver;
	~Pass_pebble_mks_all() {};

private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class Pass_pebble_soc_all : public ISolver
{
public:
	using ISolver::ISolver;
	~Pass_pebble_soc_all() {};

private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

#endif
