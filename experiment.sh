#!/bin/bash

timeout=100

for instance in instances/scenarios/random16-1.scen
do
	for encoding in at_parallel_mks_all at_parallel_soc_all pass_parallel_mks_all pass_parallel_soc_all shift_parallel_mks_all shift_parallel_soc_all 
	do
		./release/MAPF -s $instance -t $timeout -e $encoding -a 1 -i 1 -l 1 -f results.res
	done
done
