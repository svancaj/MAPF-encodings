#!/bin/bash

timeout=1000

for instance in instances/scenarios/random_20_0.scen
do
	for vars in at pass shift #monosat-pass monosat-shift
	do
		for movement in parallel pebble
		do
			for cost in soc
			do
				for lazy in eager lazy
				do
					for dup in single dupli
					do
						encoding="${cost}_${movement}_${vars}_${lazy}_${dup}"
						./release/MAPF -m instances/maps -s $instance -t $timeout -e $encoding -l 1 -f results.res -q -a 20
					done
				done
			done
		done
	done
done
