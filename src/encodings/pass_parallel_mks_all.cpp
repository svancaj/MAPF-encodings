#include "pass_parallel_mks_all.hpp"

using namespace std;

int Pass_parallel_mks_all::Solve(int agents)
{
	int delta = 0;


	while (true)
	{
		PrintSolveDetails(agents, delta);

		// create formula

		// solve formula
		int res = 1; // 0 = ok, 1 = timeout

		if (res == 0) // ok
		{
			// fill out statistic
			break;
		}
		if (res == 1) // timeout
			return 1;
		delta++; // no solution with given limits, increase delta
	}

	return 0;
}