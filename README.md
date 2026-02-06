# SAT encodings for MAPF problem

This repository contains numerous encodings for solving the multi-agent pathfinding problem (MAPF).

## Prerequisities

The system makes use of the following repositories

- CaDiCaL - https://github.com/arminbiere/cadical
- monosat - https://github.com/sambayless/monosat
- PBlib - https://github.com/master-keying/pblib

The static libraries of these tools are provided in this repository in the `libs` directory. However, for optimal experience, we encourage the user to compile the libraries themselves.

Using *monosat* C++ API is planned for future, it is not yet supported. The current implementation uses the *monosat* binary. Ignore the follwoing note.

Note that *monosat* requires *zlib* and *gmp*. To use the makefile and to compile the code, *make* and *g++* compatible with C++11 or higher are required. You can install all of those using the following:

```
sudo apt install make
sudo apt install g++
sudo apt install zlib1g-dev
sudo apt install libgmp3-dev
```

## Building the system

To build the executable binary and the library:

```
make
```

To build the library only:

```
make lib
```

To build the provided example:

```
make example
```

## Usage

### Binary

The compiled binary `MAPF` is located in the `release` directory. The expected input file format is in accordance with the [MAPF benchmark set]( https://movingai.com/benchmarks/mapf/index.html).

```
./MAPF [-h] [-q] [-p] -e encoding -s scenario_file [-m map_dir] [-a number_of_agents] [-i increment] [-t timeout] [-d delta] [-o] [-f log_file]
        -h                  : Prints help and exits
        -q                  : Suppress print on stdout
        -p                  : Print found plan. If q flag is set, p flag is overwritten.
        -e encoding         : Encoding to be used. Available options are {mks|soc}_{parallel|pebble}_{at|pass|shift|monosat-pass|monosat-shift}_{eager|lazy}_{single|dupli}
        -s scenario_file    : Path to a scenario file
        -m map_dir          : Directory containing map files. Default is instances/maps
        -a number_of_agents : Number of agents to solve. If not specified, all agents in the scenario file are used.
        -i increment        : After a successful call, increase the number of agents by the specified increment. If not specified, do not perform subsequent calls.
        -t timeout          : Timeout of the computation in seconds. Default value is 300s
        -d delta            : Cost of delta is added to the first call. Default is 0.
        -o                  : Oneshot solving. Ie. do not increment cost in case of unsat call. Default is to optimize.
        -f log_file         : log file. If not specified, output to stdout.
        -l log_level        : logging option. 0 = no log, 1 = inline log, 2 = human readable log. if -f log_file is not specified in combination with -l 1 or -l 2 overrides -q. Default is 0.
        -c cnf_file         : print the created CNF into cnf_file. If not specified, the created CNF is not printed.
```

### Library

The compiled static library `libmapf.a` is located in `release/libs`, along with the static libraries of *CaDiCaL*, *monosat*, and *PB*.

The interface of the library is in `MAPF.hpp` located in the `release` directory. An example of usage of the interface is provided in `release/example.cpp`.


## Implemented encodings

Currently, the following ideas are implemented:

### Variables
- At(a,v,t) variables only describing location *v* of agent *a* in timestep *t*
- Pass(a,v,u,t) variables with At variables describing movement of agent *a* over edge *(v,u)* at timestep *t*
- Shift(v,u,t) variables with At variables describing movement over edge *(v,u)* at timestep *t*

### Graph propagators
- Monosat-Pass creates a TEG for each agent. Makes use of At and Pass varaibles.
- Monosat-Shift creates a single TEG shared among all agents. Makes use of only Shift variables.

### Motion
- Parallel motion allows two agents to move in close proximity (also called following conflict or 0-robust solution).
- Pebble motion allows moving into vertex only if it is unoccupied in the previous timestep (also called 1-robust solution).

### Cost function
- Makespan is the first timestep all of the agents reach their goal location.
- Sum of costs is the sum of individual plan lengths.

### Lazy conflicts
- Eager encodes all conflict constraints at once.
- Lazy encodes conflict constraints only if the found plan contains the given conflict and then solves it again.

### Duplicated agents
- Single forces every agent to be present only in a single location at a time.
- Dupli allows agent do be duplicated in some timesteps.

## Publications
TBD
