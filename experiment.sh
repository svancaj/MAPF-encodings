#!/bin/bash

timeout=30

for instance in instances/scenarios/random32-1.scen
do
	for vars in monosat at pass shift
	do
		for movement in parallel #pebble
		do
			for cost in mks #soc 
			do
				for lazy in all
				do
					encoding="${vars}_${movement}_${cost}_${lazy}"
					./release/MAPF -s $instance -t $timeout -e $encoding -a 5 -i 5 -l 1 -f results.res -c formula.cnf
				done
			done
		done
	done
done
