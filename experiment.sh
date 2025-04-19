#!/bin/bash

timeout=60

for instance in instances/scenarios/random_20_1.scen
do
	for vars in monosat-pass #at pass shift
	do
		for movement in parallel #pebble
		do
			for cost in mks #soc
			do
				for lazy in all # lazy 
				do
					encoding="${vars}_${movement}_${cost}_${lazy}"
					./release/MAPF -s $instance -t $timeout -e $encoding -a 5 -i 5 -l 1 -f results.res
				done
			done
		done
	done
done
