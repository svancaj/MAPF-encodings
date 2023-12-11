#!/bin/bash

timeout=100

for instance in instances/scenarios/*
do
	for encoding in pass_parallel_mks_all pass_pebble_mks_all pass_parallel_soc_all pass_pebble_soc_all
	do
		./build/MAPF -s $instance -t $timeout -e $encoding -a 1 -i 1
	done
done
