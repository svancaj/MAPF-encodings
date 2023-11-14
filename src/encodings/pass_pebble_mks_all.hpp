#pragma once

#include "solver_common.hpp"

class Pass_pebble_mks_all : public ISolver
{
public:
	using ISolver::ISolver;
	~Pass_pebble_mks_all() {};
	
	int Solve(int);

private:
	int CreateFormula(std::vector<std::vector<int> >&, int);
};