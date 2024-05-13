#ifndef _solver_common_h_INCLUDED
#define _solver_common_h_INCLUDED

#include <unistd.h>
#include <chrono>
#include <thread>

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
    * @param oneshot option to perform just one solver call without optimization. Default is false.
    */
    int Solve(int, int = 0, bool = false);

    /** Set data before solving.
    * 
    * Should be performed before the first solve. The stored data will be remembered for all of the solve calls.
    *
    * @param instance pointer to a _MAPFSAT_Instance.
    * @param logger pointer to a _MAPFSAT_Logger.
    * @param timeout timeout for each solve call in [s]. Gets reseted after each solve call.
    * @param quiet option to suppress any print to stdout. Default is false.
    * @param print_paths option to print found paths. Default is false.
    */
    void SetData(_MAPFSAT_Instance*, _MAPFSAT_Logger*, int, bool = false, bool = false, bool = false);

protected:
	_MAPFSAT_Instance* inst;
	_MAPFSAT_Logger* log;
	std::string solver_name;
	int cost_function; // 1 = mks, 2 = soc
	int movement; // 1 = parallel, 2 = pebble
	int lazy_const; // 1 = all at once, 2 = lazy
	int timeout;
	bool quiet;
	bool print_plan;
	bool use_avoid;

	int agents;
	int vertices;
	int delta;
	int max_timestep;

	int variables;

	_MAPFSAT_TEGAgent** at;
	_MAPFSAT_TEGAgent*** pass;
	_MAPFSAT_Shift** shift;
	int* shift_times_start;
	int* shift_times_end;
	int at_vars;

	std::vector<std::vector<int> > CNF;

	int nr_vars;
	int nr_clauses;
	int solver_calls;

	// before solving
	void PrintSolveDetails(int);
	void DebugPrint(std::vector<std::vector<int> >& );

	// virtual encoding to be used
	virtual int CreateFormula(std::vector<std::vector<int> >&, int) = 0;

	// creating formula
	int CreateAt(int, int);
	int CreatePass(int, int);
	int CreateShift(int, int);

	void CreatePossition_Start(std::vector<std::vector<int> >&);
	void CreatePossition_Goal(std::vector<std::vector<int> >&);
	void CreatePossition_NoneAtGoal(std::vector<std::vector<int> >&);

	void CreateConf_Vertex(std::vector<std::vector<int> >&);
	void CreateConf_Swapping_At(std::vector<std::vector<int> >&);
	void CreateConf_Swapping_Pass(std::vector<std::vector<int> >&);
	void CreateConf_Swapping_Shift(std::vector<std::vector<int> >&);
	void CreateConf_Pebble_At(std::vector<std::vector<int> >&);
	void CreateConf_Pebble_Pass(std::vector<std::vector<int> >&);
	void CreateConf_Pebble_Shift(std::vector<std::vector<int> >&);

	void CreateMove_NoDuplicates(std::vector<std::vector<int> >&);
	void CreateMove_NextVertex_At(std::vector<std::vector<int> >&);
	void CreateMove_EnterVertex_Pass(std::vector<std::vector<int> >&);
	void CreateMove_LeaveVertex_Pass(std::vector<std::vector<int> >&);
	void CreateMove_ExactlyOne_Shift(std::vector<std::vector<int> >&);
	void CreateMove_NextVertex_Shift(std::vector<std::vector<int> >&);

	int CreateConst_LimitSoc(std::vector<std::vector<int> >&, int);
	void CreateConst_Avoid(std::vector<std::vector<int> >&);


	// solving
	int InvokeSolver(std::vector<std::vector<int> >&, int);
	int normalizePlan(std::vector<std::vector<int> >&);
	void verifyPlan(std::vector<std::vector<int> >&);
	static void wait_for_terminate(int, void*, bool&);
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
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_AtParallelSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtParallelSocAll(std::string name = "at_parallel_soc_all");
	~_MAPFSAT_AtParallelSocAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_AtPebbleMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtPebbleMksAll(std::string name = "at_pebble_mks_all");
	~_MAPFSAT_AtPebbleMksAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_AtPebbleSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_AtPebbleSocAll(std::string name = "at_pebble_soc_all");
	~_MAPFSAT_AtPebbleSocAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_PassParallelMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassParallelMksAll(std::string name = "pass_parallel_mks_all");
	~_MAPFSAT_PassParallelMksAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_PassParallelSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassParallelSocAll(std::string name = "pass_parallel_soc_all");
	~_MAPFSAT_PassParallelSocAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_PassPebbleMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassPebbleMksAll(std::string name = "pass_pebble_mks_all");
	~_MAPFSAT_PassPebbleMksAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_PassPebbleSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_PassPebbleSocAll(std::string name = "pass_pebble_soc_all");
	~_MAPFSAT_PassPebbleSocAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_ShiftParallelMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftParallelMksAll(std::string name = "shift_parallel_mks_all");
	~_MAPFSAT_ShiftParallelMksAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_ShiftParallelSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftParallelSocAll(std::string name = "shift_parallel_soc_all");
	~_MAPFSAT_ShiftParallelSocAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_ShiftPebbleMksAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftPebbleMksAll(std::string name = "shift_pebble_mks_all");
	~_MAPFSAT_ShiftPebbleMksAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

class _MAPFSAT_ShiftPebbleSocAll : public _MAPFSAT_ISolver
{
public:
	_MAPFSAT_ShiftPebbleSocAll(std::string name = "shift_pebble_soc_all");
	~_MAPFSAT_ShiftPebbleSocAll() {};
private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};

#endif
