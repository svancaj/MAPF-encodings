#ifndef _solver_common_h_INCLUDED
#define _solver_common_h_INCLUDED

#include <unistd.h>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <tuple>
#include <cassert>

#include "../instance.hpp"
#include "../logger.hpp"

struct _MAPFSAT_TEGAgent
{
	int first_variable;
	int first_timestep;
	int last_timestep;
};

struct _MAPFSAT_Shift
{
	int first_varaible;
	std::vector<int> timestep;
};

class _MAPFSAT_ISolver
{
public:
    virtual ~_MAPFSAT_ISolver() {};
    
    /** Perform a solve call.
    * 
    * Creates CNF formula based on the specified encoding and calls SAT solver to find a solution.
    *
    * @param ags number of agents in the solve call.
    * @param delta the initial delta. Default is 0.
    * @param oneshot option to perform just one solver call with the given delta without incrementing. Default is false.
	* @param keep_plan option save the found plan. The found plan can be retrieved by GetPlan function. Default is false.
	* @return -1 unSAT with given cost, 0 valid solution, 1 timeout or error.
    */
    int Solve(int, int = 0, bool = false, bool = false);

    /** Set data before solving.
    * 
    * Should be performed before the first solve. The stored data will be remembered for all of the solve calls.
    *
    * @param instance pointer to a _MAPFSAT_Instance.
    * @param logger pointer to a _MAPFSAT_Logger.
    * @param timeout timeout for each solve call in [s]. Gets reseted after each solve call.
	* @param CNF_file file name to print the created CNF formula. If no file is specified, the formula is not printed. Default is "".
    * @param quiet option to suppress any print to stdout. Default is false.
    * @param print_paths option to print found paths. Default is false.
	* @param use_avoids option to avoid certain postions in time. The avoid data is stored in the _MAPFSAT_Instance class. Default is false.
    */
    void SetData(_MAPFSAT_Instance*, _MAPFSAT_Logger*, int, std::string = "", bool = false, bool = false, bool = false);

	/** Returns the found plan.
    * 
    * Returns the found plan if the solve was successful and keep_plan was set to true.
    *
    */
	std::vector<std::vector<int> > GetPlan();

protected:
	_MAPFSAT_Instance* inst;
	_MAPFSAT_Logger* log;
	std::string solver_name;
	int variables; // 1 = at, 2 = pass, 3 = shift
	int cost_function; // 1 = mks, 2 = soc
	int movement; // 1 = parallel, 2 = pebble
	int lazy_const; // 1 = all at once, 2 = lazy
	int timeout;
	std::string cnf_file;
	bool quiet;
	bool print_plan;
	bool use_avoid;
	bool keep_plan;
	int solver_to_use = 1; // 1 = kissat, 2 = monosat
	int duplicates; // 1 = forbid, 2 = allow

	int agents;
	int vertices;
	int delta;
	int max_timestep;
	std::stringstream cnf_printable;

	_MAPFSAT_TEGAgent** at;
	_MAPFSAT_TEGAgent*** pass;
	_MAPFSAT_Shift** shift;
	int* shift_times_start;
	int* shift_times_end;
	int at_vars;

	int nr_vars;
	long long nr_clauses;
	long long nr_clauses_move;
	long long nr_clauses_conflict;
	long long nr_clauses_soc;
	long long nr_clauses_assumption;
	int solver_calls;

	std::vector<std::vector<int> > plan;

	bool conflicts_present;

	std::vector<std::tuple<int,int,int,int> > vertex_conflicts;
	std::vector<std::tuple<int,int,int,int,int> > swap_conflicts;
	std::vector<std::tuple<int,int,int,int,int> > pebble_conflicts;

	// solver
	void* SAT_solver;

	// before solving
	void PrintSolveDetails(int);

	// virtual encoding to be used
	virtual int CreateFormula(int) = 0;

	// creating formula
	int CreateAt(int, int);
	int CreatePass(int, int);
	int CreateShift(int, int);

	void CreatePossition_Start();
	void CreatePossition_Goal();
	void CreatePossition_NoneAtGoal();
	void CreatePossition_NoneAtGoal_Shift();

	void CreateConf_Vertex();
	void CreateConf_Swapping_At();
	void CreateConf_Swapping_Pass();
	void CreateConf_Swapping_Shift();
	void CreateConf_Pebble_At();
	void CreateConf_Pebble_Pass();
	void CreateConf_Pebble_Shift();

	void CreateConf_Vertex_OnDemand();
	void CreateConf_Swapping_At_OnDemand();
	void CreateConf_Swapping_Pass_OnDemand();
	void CreateConf_Swapping_Shift_OnDemand();
	void CreateConf_Pebble_At_OnDemand();
	void CreateConf_Pebble_Pass_OnDemand();
	void CreateConf_Pebble_Shift_OnDemand();

	void CreateMove_NoDuplicates();
	void CreateMove_NextVertex_At();
	void CreateMove_EnterVertex_Pass();
	void CreateMove_LeaveVertex_Pass();
	void CreateMove_NextEdge_Pass();
	void CreateMove_ExactlyOne_Shift();
	void CreateMove_ExactlyOneIncoming_Shift();
	void CreateMove_NextVertex_Shift();

	int CreateConst_LimitSoc(int);
	int CreateConst_LimitSoc_AllAt(int);
	int CreateConst_LimitSoc_Shift(int);
	void CreateConst_Avoid();

	// solver functions
	virtual void AddClause(std::vector<int>) = 0;
	virtual void CreateSolver() = 0;
	virtual void ReleaseSolver() = 0;
	int InvokeSolver(int);
	virtual int InvokeSolverImplementation(int) = 0;

	// plan outputting functions
	int NormalizePlan();
	void PrintPlan();
	void VerifyPlan();
	void GenerateConflicts();

	// cleanup functions
	bool TimesUp(std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>, int);
	void CleanUp();
};

/******************************************************************/
/*********************** Specific Encodings ***********************/
/******************************************************************/

class _MAPFSAT_SAT : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_SAT(int, int, int, int, int, int solver = 1, std::string name = "SAT_encoding");
	~_MAPFSAT_SAT() {};
private:
	int CreateFormula(int);

	void AddClause(std::vector<int>);
	void CreateSolver();
	void ReleaseSolver();
	int InvokeSolverImplementation(int);

	// specialized functions
	static void WaitForTerminate(int, void*, bool&);
	int GetNextVertex(int, int, int);
};

class _MAPFSAT_SMT : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_SMT(int, int, int, int, int, int solver = 2, std::string name = "SMT_encoding");
	~_MAPFSAT_SMT() {};
private:
	int CreateFormula(int);

	void AddClause(std::vector<int>);
	void CreateSolver();
	void ReleaseSolver();
	int InvokeSolverImplementation(int);

	// specialized functions
	int CreateMove_Graph_MonosatPass(int);
	int CreateMove_Graph_MonosatShift(int);
	int VarToID(int, bool, int&, std::unordered_map<int, int>&);
	int GetNextVertex(std::vector<bool>&, int, int, int);
};

#endif
