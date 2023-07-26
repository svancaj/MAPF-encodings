#pragma once

#include "isolver.hpp"

class Pass_parallel_mks_all : public ISolver
{
public:
	using ISolver::ISolver;
	~Pass_parallel_mks_all() {};
	
	int Solve(int);

private:
	
};