#!/bin/bash

timeout=120

for instance in instances/testing/scenarios/*
do
	for vars in at pass shift
	do
		for movement in parallel pebble
		do
			for cost in mks soc
			do
				for lazy in eager lazy
				do
					for dup in single #dupli 
					do
						encoding="${cost}_${movement}_${vars}_${lazy}_${dup}"
						./release/MAPF -m instances/testing/maps -s $instance -t $timeout -e $encoding -l 1 -f results.res
					done
				done
			done
		done
	done
done
