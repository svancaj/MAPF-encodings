#pragma once

#include <unistd.h>

#include "../instance.hpp"
#include "../logger.hpp"

class ISolver
{
public:
	ISolver(Instance* i, Logger* l, std::string sn) : inst(i), log(l), solver_name(sn) { std::cout << "create" << std::endl; }; 
	
	virtual int Solve(int) = 0;

protected:
	std::string solver_name;
	Instance* inst;
	Logger* log;
};

