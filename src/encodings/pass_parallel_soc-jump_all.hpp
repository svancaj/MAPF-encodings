#pragma once

#include "solver_common.hpp"

class Pass_parallel_socjump_all : public ISolver
{
public:
	using ISolver::ISolver;
	~Pass_parallel_socjump_all() {};
	
	int Solve(int);

private:
	int CreateMksFormula(std::vector<std::vector<int> >&, int);
	int CreateSocFormula(std::vector<std::vector<int> >&, int);
	int CreateDeltaFormula(std::vector<std::vector<int> >&, int, int);
};