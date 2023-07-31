#pragma once

#include <unistd.h>
#include <chrono>

#include "../instance.hpp"
#include "../logger.hpp"

class ISolver
{
public:
	ISolver(std::string sn, int opt) : solver_name(sn), cost_function(opt) { std::cout << "create" << std::endl; }; 
	virtual ~ISolver() {};
	
	virtual int Solve(int) = 0;

	void SetData(Instance* i, Logger* l)
	{
		inst = i;
		log = l;
	};

protected:
	Instance* inst;
	Logger* log;
	std::string solver_name;
	int cost_function; // 1 = mks, 2 = soc

	std::vector<std::vector<int> > CNF;

	void PrintSolveDetails(int ags, int delta)
	{
		std::cout << "Currently solving" << std::endl;
		std::cout << "Instance name: " << inst->scen_name << std::endl;
		std::cout << "Map name: " << inst->map_name << std::endl;
		std::cout << "Encoding used: " << solver_name << std::endl;
		std::cout << "Optimizing function: " << ((cost_function == 1) ? "makespan" : "sum of costs") << std::endl;
		std::cout << "Number of agents: " << ags << std::endl;
		std::cout << "Mks LB: " << inst->GetMksLB(ags) << std::endl;
		std::cout << "SoC LB: " << inst->GetSocLB(ags) << std::endl;
		std::cout << "Delta: " << delta << std::endl;
		std::cout << std::endl;
	};

	void DebugPrint(std::vector<std::vector<int> >& CNF)
	{
		for (size_t i = 0; i < CNF.size(); i++)
		{
			for (size_t j = 0; j < CNF[i].size(); j++)
			{
				std::cout << CNF[i][j] << " ";
			}
			std::cout << "0\n";
		}
		std::cout << "0" << std::endl;
	}
};

