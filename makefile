CC = g++
CFLAGS = -std=c++11 -O3 -Wall -Wextra -pedantic
S_DIR = src
E_DIR = $(S_DIR)/encodings
B_DIR = build
PROJECT_NAME = MAPF

_LIBS = libpb.a libkissat.a
LIBS = $(patsubst %,$(B_DIR)/%,$(_LIBS))

_ENC_DEPS = solver_common.hpp
_DEPS = instance.hpp logger.hpp 
DEPS = $(patsubst %,$(S_DIR)/%,$(_DEPS)) $(patsubst %,$(E_DIR)/%,$(_ENC_DEPS))

_ENC_OBJ = solver_common.o pass_parallel_mks_all.o pass_parallel_soc_all.o pass_pebble_mks_all.o
_OBJ = main.o instance.o logger.o
OBJ = $(patsubst %,$(B_DIR)/%,$(_OBJ)) $(patsubst %, $(B_DIR)/%,$(_ENC_OBJ))

_OBJ_USECASE = usecase.o instance.o logger.o
OBJ_USECASE = $(patsubst %,$(B_DIR)/%,$(_OBJ_USECASE)) $(patsubst %, $(B_DIR)/%,$(_ENC_OBJ))

$(PROJECT_NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $(B_DIR)/$@ $^ $(LIBS)

$(B_DIR)/%.o: $(E_DIR)/%.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(B_DIR)/%.o: $(S_DIR)/%.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(B_DIR)/*.o $(B_DIR)/$(PROJECT_NAME) log.log $(B_DIR)/usecase

test: $(PROJECT_NAME)
	$(B_DIR)/$(PROJECT_NAME) -s instances/testing/scenarios/test2.scen -e pass_pebble_mks_all -t 30 -a 2

experiment: $(PROJECT_NAME)
	sh experiment.sh

usecase: $(OBJ_USECASE)
	$(CC) $(CFLAGS) -o $(B_DIR)/$@ $^ $(LIBS)

test_usecase: usecase
	$(B_DIR)/$^