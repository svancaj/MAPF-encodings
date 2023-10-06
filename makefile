CC = g++
CFLAGS = -std=c++11 -O3 -Wall -Wextra -pedantic
S_DIR = src
E_DIR = $(S_DIR)/encodings
B_DIR = build

_LIBS = libpb.a libkissat.a
LIBS = $(patsubst %,$(B_DIR)/%,$(_LIBS))

_ENC_DEPS = solver_common.hpp pass_parallel_mks_all.hpp pass_parallel_soc_all.hpp
_DEPS = instance.hpp logger.hpp 
DEPS = $(patsubst %,$(S_DIR)/%,$(_DEPS)) $(patsubst %,$(E_DIR)/%,$(_ENC_DEPS))

_ENC_OBJ = solver_common.o pass_parallel_mks_all.o pass_parallel_soc_all.o
_OBJ = main.o instance.o logger.o
OBJ = $(patsubst %,$(B_DIR)/%,$(_OBJ)) $(patsubst %, $(B_DIR)/%,$(_ENC_OBJ))

MAPF: $(OBJ)
	$(CC) $(CFLAGS) -o $(B_DIR)/$@ $^ $(LIBS)

$(B_DIR)/%.o: $(E_DIR)/%.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(B_DIR)/%.o: $(S_DIR)/%.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(B_DIR)/*.o $(B_DIR)/MAPF

test: MAPF
	$(B_DIR)/MAPF -s instances/testing/scenarios/test1.scen -e pass_parallel_soc_all -t 30 -a 2

experiment: MAPF
	sh experiment.sh
