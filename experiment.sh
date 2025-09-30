#!/bin/bash

timeout=60

for instance in instances/scenarios/random_20_*
do
	for vars in pass at #shift
	do
		for movement in parallel pebble
		do
			for cost in soc mks 
			do
				for lazy in all lazy 
				do
					encoding="${vars}_${movement}_${cost}_${lazy}"
					./release/MAPF -s $instance -t $timeout -e $encoding -a 5 -i 5 -l 1 -f results.res -c tmp.cnf
				done
			done
		done
	done
done
