#pragma once

#include "isolver.hpp"

class Pass_parallel_mks_all : public ISolver
{
public:
	using ISolver::ISolver;
	int Solve(int);

private:
	
};