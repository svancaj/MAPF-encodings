CC = g++
CFLAGS = -std=c++11 -O3 -Wall -Wextra -pedantic
S_DIR = src
E_DIR = $(S_DIR)/encodings
B_DIR = build
PROJECT_NAME = MAPF

_LIBS = libpb.a libkissat.a
LIBS = $(patsubst %,$(B_DIR)/%,$(_LIBS))

_DEPS = instance.hpp logger.hpp encodings/solver_common.hpp
DEPS = $(patsubst %,$(S_DIR)/%,$(_DEPS))

VAR = at pass
MOVE = parallel pebble
FUNC = mks soc
COMP = all

_ENC_OBJ = solver_common.o $(foreach v, $(VAR), $(foreach m, $(MOVE), $(foreach f, $(FUNC), $(foreach c, $(COMP), $v_$m_$f_$c.o))))
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
	rm -f $(B_DIR)/*.o $(B_DIR)/$(PROJECT_NAME) valgrind-out.txt log.log $(B_DIR)/usecase

test: $(PROJECT_NAME)
	$(B_DIR)/$(PROJECT_NAME) -s instances/testing/scenarios/test2.scen -e at_pebble_soc_all -t 30 -a 2 -p

valgrind: $(PROJECT_NAME)
	valgrind --leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--verbose \
	--log-file=valgrind-out.txt \
	$(B_DIR)/$(PROJECT_NAME) -s instances/testing/scenarios/test2.scen -e pass_pebble_soc_all -t 30 -a 2 -p

experiment: $(PROJECT_NAME)
	sh experiment.sh

usecase: $(OBJ_USECASE)
	$(CC) $(CFLAGS) -o $(B_DIR)/$@ $^ $(LIBS)

test_usecase: usecase
	$(B_DIR)/$^