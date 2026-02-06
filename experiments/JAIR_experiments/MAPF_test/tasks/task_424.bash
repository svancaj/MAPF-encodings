#!/bin/bash
#PBS -l select=1:ncpus=1:mem=64gb:scratch_local=10gb:cl_turin=True
#PBS -l walltime=48:00:00

PATH_PREFIX=/storage/praha1/home/svancaj/MAPF_test
TEST_NAME=MAPF_Encodings
TASK_NR=424

trap 'clean_scratch' TERM EXIT

#copy the program to SCRATCHDIR and access the folder
cp -RL $PATH_PREFIX/$TEST_NAME $SCRATCHDIR/$TEST_NAME
cd $SCRATCHDIR

uname -a >> $PATH_PREFIX/run.log
echo "scratchdir=$SCRATCHDIR"
date

timeout=300

INST=empty_70_3.scen
instance=$TEST_NAME/instances/scenarios/$INST

for vars in at pass shift #monosat-pass monosat-shift
do
	for movement in parallel pebble
	do
		for cost in soc #mks
		do
			for lazy in eager lazy
			do
				for dup in single dupli
				do
					encoding="${cost}_${movement}_${vars}_${lazy}_${dup}"
					echo $encoding >&2
					date
					echo $encoding
					timeout 7200 ./$TEST_NAME/release/MAPF -m $TEST_NAME/instances/maps -s $instance -t $timeout -e $encoding -a 5 -i 5 -q -l 1 -f results_$TASK_NR.res
				done
			done
		done
	done
done

date

mkdir -p $PATH_PREFIX/results
cp -r $SCRATCHDIR/results* $PATH_PREFIX/results/
