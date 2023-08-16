CC = g++
CFLAGS = -std=c++11 -O3 -Wall -Wextra -pedantic
S_DIR = src
E_DIR = $(S_DIR)/encodings
B_DIR = build


_ENC_DEPS = solver_common.hpp pass_parallel_mks_all.hpp
_DEPS = instance.hpp logger.hpp 
DEPS = $(patsubst %,$(S_DIR)/%,$(_DEPS)) $(patsubst %,$(E_DIR)/%,$(_ENC_DEPS))

_ENC_OBJ = solver_common.o pass_parallel_mks_all.o
_OBJ = main.o instance.o logger.o
OBJ = $(patsubst %,$(abspath $(S_DIR))/%,$(_OBJ)) $(patsubst %,$(abspath $(E_DIR))/%,$(_ENC_OBJ))

MAPF: $(OBJ)
	mkdir -p build
	mkdir -p run
	$(CC) $(CFLAGS) -o $(B_DIR)/$@ $^

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(S_DIR)/*.o $(E_DIR)/*.o $(B_DIR)/MAPF

test: MAPF
	$(B_DIR)/MAPF -s instances/scenarios/empty08-1.scen -e pass_parallel_mks_all -a 50

experiment: MAPF
	sh experiment.sh
