#!/bin/bash

timeout=300

for instance in instances/scenarios/random08-1.scen instances/scenarios/random32-1.scen instances/scenarios/random64-1.scen
do
	for vars in at pass shift #monosat
	do
		for movement in parallel pebble
		do
			for cost in mks soc
			do
				for lazy in lazy all 
				do
					encoding="${vars}_${movement}_${cost}_${lazy}"
					./release/MAPF -s $instance -t $timeout -e $encoding -a 5 -i 5 -l 1 -f results.res
				done
			done
		done
	done
done
