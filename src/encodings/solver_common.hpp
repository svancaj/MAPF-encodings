#ifndef _solver_common_h_INCLUDED
#define _solver_common_h_INCLUDED

#include <unistd.h>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <tuple>

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
    * Returns the found plan if the solve was success and keep_plan was set to true
    *
    */
	std::vector<std::vector<int> > GetPlan();

protected:
	_MAPFSAT_Instance* inst;
	_MAPFSAT_Logger* log;
	std::string solver_name;
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
	int nr_clauses;
	int nr_clauses_move;
	int nr_clauses_conflict;
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
	void CreateMove_ExactlyOne_Shift();
	void CreateMove_ExactlyOneIncoming_Shift();
	void CreateMove_NextVertex_Shift();

	int CreateMove_Graph_MonosatPass(int);
	int CreateMove_Graph_MonosatShift(int);

	int CreateConst_LimitSoc(int);
	int CreateConst_LimitSoc_AllAt(int);
	int CreateConst_LimitSoc_Shift(int);
	void CreateConst_Avoid();

	// solving
	int InvokeSolver_Kissat(int);
	int InvokeSolver_Monosat(int);
	
	// auxiliary SAT solver functions
	void CreateSolver();
	int InvokeSolver(int);
	static void WaitForTerminate(int, void*, bool&);
	int VarToID(int, bool, int&, std::unordered_map<int, int>&);
	void AddClause(std::vector<int>);

	// plan outputting functions
	int NormalizePlan();
	void PrintPlan();
	void VerifyPlan();
	void GenerateConflicts();

	// cleanup functions
	bool TimesUp(std::chrono::time_point<std::chrono::high_resolution_clock>, std::chrono::time_point<std::chrono::high_resolution_clock>, int);
	void CleanUp(bool);
};

/******************************************************************/
/*********************** Specific Encodings ***********************/
/******************************************************************/

class _MAPFSAT_AtParallelMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtParallelMksAll(std::string name = "at_parallel_mks_all");
	~_MAPFSAT_AtParallelMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_AtParallelSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtParallelSocAll(std::string name = "at_parallel_soc_all");
	~_MAPFSAT_AtParallelSocAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_AtPebbleMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtPebbleMksAll(std::string name = "at_pebble_mks_all");
	~_MAPFSAT_AtPebbleMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_AtPebbleSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtPebbleSocAll(std::string name = "at_pebble_soc_all");
	~_MAPFSAT_AtPebbleSocAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_PassParallelMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassParallelMksAll(std::string name = "pass_parallel_mks_all");
	~_MAPFSAT_PassParallelMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_PassParallelSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassParallelSocAll(std::string name = "pass_parallel_soc_all");
	~_MAPFSAT_PassParallelSocAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_PassPebbleMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassPebbleMksAll(std::string name = "pass_pebble_mks_all");
	~_MAPFSAT_PassPebbleMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_PassPebbleSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassPebbleSocAll(std::string name = "pass_pebble_soc_all");
	~_MAPFSAT_PassPebbleSocAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_ShiftParallelMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftParallelMksAll(std::string name = "shift_parallel_mks_all");
	~_MAPFSAT_ShiftParallelMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_ShiftParallelSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftParallelSocAll(std::string name = "shift_parallel_soc_all");
	~_MAPFSAT_ShiftParallelSocAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_ShiftPebbleMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftPebbleMksAll(std::string name = "shift_pebble_mks_all");
	~_MAPFSAT_ShiftPebbleMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_ShiftPebbleSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftPebbleSocAll(std::string name = "shift_pebble_soc_all");
	~_MAPFSAT_ShiftPebbleSocAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_AtParallelMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtParallelMksLazy(std::string name = "at_parallel_mks_lazy");
	~_MAPFSAT_AtParallelMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_AtParallelSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtParallelSocLazy(std::string name = "at_parallel_soc_lazy");
	~_MAPFSAT_AtParallelSocLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_AtPebbleMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtPebbleMksLazy(std::string name = "at_pebble_mks_lazy");
	~_MAPFSAT_AtPebbleMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_AtPebbleSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtPebbleSocLazy(std::string name = "at_pebble_soc_lazy");
	~_MAPFSAT_AtPebbleSocLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_PassParallelMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassParallelMksLazy(std::string name = "pass_parallel_mks_lazy");
	~_MAPFSAT_PassParallelMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_PassParallelSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassParallelSocLazy(std::string name = "pass_parallel_soc_lazy");
	~_MAPFSAT_PassParallelSocLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_PassPebbleMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassPebbleMksLazy(std::string name = "pass_pebble_mks_lazy");
	~_MAPFSAT_PassPebbleMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_PassPebbleSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassPebbleSocLazy(std::string name = "pass_pebble_soc_lazy");
	~_MAPFSAT_PassPebbleSocLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_ShiftParallelMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftParallelMksLazy(std::string name = "shift_parallel_mks_lazy");
	~_MAPFSAT_ShiftParallelMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_ShiftParallelSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftParallelSocLazy(std::string name = "shift_parallel_soc_lazy");
	~_MAPFSAT_ShiftParallelSocLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_ShiftPebbleMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftPebbleMksLazy(std::string name = "shift_pebble_mks_lazy");
	~_MAPFSAT_ShiftPebbleMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_ShiftPebbleSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftPebbleSocLazy(std::string name = "shift_pebble_soc_lazy");
	~_MAPFSAT_ShiftPebbleSocLazy() {};
private:
	int CreateFormula(int);
};

/*****************************************************************/
/*********************** Monosat Encodings ***********************/
/*****************************************************************/

class _MAPFSAT_MonosatPassParallelMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatPassParallelMksAll(std::string name = "monosat-pass_parallel_mks_all");
	~_MAPFSAT_MonosatPassParallelMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatPassParallelSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatPassParallelSocAll(std::string name = "monosat-pass_parallel_soc_all");
	~_MAPFSAT_MonosatPassParallelSocAll() {};
private:
	int CreateFormula(int);
};

/*class _MAPFSAT_MonosatPassPebbleMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatPassPebbleMksAll(std::string name = "monosat-pass_pebble_mks_all");
	~_MAPFSAT_MonosatPassPebbleMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatPassPebbleSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatPassPebbleSocAll(std::string name = "monosat-pass_pebble_soc_all");
	~_MAPFSAT_MonosatPassPebbleSocAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatPassParallelMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatPassParallelMksLazy(std::string name = "monosat-pass_parallel_mks_lazy");
	~_MAPFSAT_MonosatPassParallelMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatPassParallelSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatPassParallelSocLazy(std::string name = "monosat-pass_parallel_soc_lazy");
	~_MAPFSAT_MonosatPassParallelSocLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatPassPebbleMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatPassPebbleMksLazy(std::string name = "monosat-pass_pebble_mks_lazy");
	~_MAPFSAT_MonosatPassPebbleMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatPassPebbleSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatPassPebbleSocLazy(std::string name = "monosat-pass_pebble_soc_lazy");
	~_MAPFSAT_MonosatPassPebbleSocLazy() {};
private:
	int CreateFormula(int);
};*/

class _MAPFSAT_MonosatShiftParallelMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatShiftParallelMksAll(std::string name = "monosat-shift_parallel_mks_all");
	~_MAPFSAT_MonosatShiftParallelMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatShiftParallelSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatShiftParallelSocAll(std::string name = "monosat-shift_parallel_soc_all");
	~_MAPFSAT_MonosatShiftParallelSocAll() {};
private:
	int CreateFormula(int);
};

/*class _MAPFSAT_MonosatShiftPebbleMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatShiftPebbleMksAll(std::string name = "monosat-shift_pebble_mks_all");
	~_MAPFSAT_MonosatShiftPebbleMksAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatShiftPebbleSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatShiftPebbleSocAll(std::string name = "monosat-shift_pebble_soc_all");
	~_MAPFSAT_MonosatShiftPebbleSocAll() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatShiftParallelMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatShiftParallelMksLazy(std::string name = "monosat-shift_parallel_mks_lazy");
	~_MAPFSAT_MonosatShiftParallelMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatShiftParallelSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatShiftParallelSocLazy(std::string name = "monosat-shift_parallel_soc_lazy");
	~_MAPFSAT_MonosatShiftParallelSocLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatShiftPebbleMksLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatShiftPebbleMksLazy(std::string name = "monosat-shift_pebble_mks_lazy");
	~_MAPFSAT_MonosatShiftPebbleMksLazy() {};
private:
	int CreateFormula(int);
};

class _MAPFSAT_MonosatShiftPebbleSocLazy : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_MonosatShiftPebbleSocLazy(std::string name = "monosat-shift_pebble_soc_lazy");
	~_MAPFSAT_MonosatShiftPebbleSocLazy() {};
private:
	int CreateFormula(int);
};*/

#endif
