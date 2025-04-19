#!/bin/bash
#PBS -l select=1:ncpus=1:mem=64gb:scratch_local=1gb:cl_turin=True
#PBS -l walltime=24:00:00

PATH_PREFIX=/storage/praha1/home/svancaj/MAPF_test
TEST_NAME=MAPF_Encodings
TASK_NR=68

trap 'clean_scratch' TERM EXIT

#copy the program to SCRATCHDIR and access the folder
cp -RL $PATH_PREFIX/$TEST_NAME $SCRATCHDIR/$TEST_NAME
cd $SCRATCHDIR

uname -a >> $PATH_PREFIX/run.log
echo "scratchdir=$SCRATCHDIR"
date

timeout=300

INST=warehouse_100_2.scen
instance=$TEST_NAME/instances/scenarios/$INST

for vars in pass shift
do
	for movement in parallel
	do
		for cost in mks soc
		do
			for lazy in all 
			do
				encoding="${vars}_${movement}_${cost}_${lazy}"
				echo $encoding >&2
				date
				echo $encoding
				timeout 7200 ./$TEST_NAME/release/MAPF -m $TEST_NAME/instances/maps -s $instance -t $timeout -e $encoding -a 5 -i 5 -q -l 1 -f results_$TASK_NR.res
			done
		done
	done
done

date

mkdir -p $PATH_PREFIX/results
cp -r $SCRATCHDIR/results* $PATH_PREFIX/results/
