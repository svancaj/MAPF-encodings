#pragma once

#include "isolver.hpp"

class Pass_parallel_mks_all : public ISolver
{
public:
	using ISolver::ISolver;
	~Pass_parallel_mks_all() {};
	
	int Solve(int);

private:
	int CreateFormula(std::vector<std::vector<int> >&, int, int);
	void PrintCNF(std::vector<std::vector<int> >&);
	int InvokeSolver();
	
};