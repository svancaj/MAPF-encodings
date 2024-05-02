#!/bin/bash

timeout=30

for instance in instances/scenarios/random16-1.scen
do
	for vars in at pass shift
	do
		for movement in parallel pebble
		do
			for cost in mks soc 
			do
				for lazy in all
				do
					encoding="${vars}_${movement}_${cost}_${lazy}"
					./release/MAPF -s $instance -t $timeout -e $encoding -a 1 -i 1 -l 1 -f results.res
				done
			done
		done
	done
done
