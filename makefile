####################
# define variables #
####################

CC = g++
CFLAGS = -std=c++11 -O3 -Wall -Wextra -pedantic
S_DIR = src
E_DIR = $(S_DIR)/encodings
O_DIR = .object_files
L_DIR = libs
R_DIR = release
PROJECT_NAME = MAPF
OUTPUT_LIB = libmapf.a
HEADER_NAME = MAPF.hpp
EX_NAME = example

_LIBS = libpb.a libkissat.a libmonosat.a
LIBS = $(patsubst %,$(L_DIR)/%,$(_LIBS))
RELEASE_LIBS = $(patsubst %,$(R_DIR)/$(L_DIR)/%,$(OUTPUT_LIB)) $(patsubst %,$(R_DIR)/$(L_DIR)/%,$(_LIBS))

_DEPS = instance.hpp logger.hpp encodings/solver_common.hpp
DEPS = $(patsubst %,$(S_DIR)/%,$(_DEPS))

VAR = at pass shift monosat
MOVE = parallel pebble
FUNC = mks soc
COMP = all

_ENC_OBJ = solver_common.o $(foreach v, $(VAR), $(foreach m, $(MOVE), $(foreach f, $(FUNC), $(foreach c, $(COMP), $v_$m_$f_$c.o))))
_OBJ = instance.o logger.o
OBJ = $(patsubst %,$(O_DIR)/%,$(_OBJ)) $(patsubst %, $(O_DIR)/%,$(_ENC_OBJ))
_MAIN = main.o
MAIN = $(patsubst %,$(O_DIR)/%,$(_MAIN))

_OBJ_EXAMPLE = $(EX_NAME).o
OBJ_EXAMPLE = $(patsubst %,$(O_DIR)/%,$(_OBJ_EXAMPLE))

###################
# project targets #
###################

all: $(PROJECT_NAME) lib example

release: $(PROJECT_NAME) lib 

# binary only
$(PROJECT_NAME): $(OBJ) $(MAIN)
	$(CC) $(CFLAGS) -o $(R_DIR)/$@ $^ $(LIBS)

# library and header only
lib: $(OBJ) $(DEPS)
#	rm -f $(R_DIR)/$(OUTPUT_LIB)
#	ar cqT $(R_DIR)/$(OUTPUT_LIB) $(OBJ) $(LIBS)
#	@echo 'create $(R_DIR)/$(OUTPUT_LIB)\naddlib $(R_DIR)/$(OUTPUT_LIB)\nsave\nend' | ar -M

	rm -rf $(R_DIR)/$(L_DIR)
	mkdir $(R_DIR)/$(L_DIR)
	cp $(L_DIR)/*.a $(R_DIR)/$(L_DIR)
	ar -rcs $(R_DIR)/$(L_DIR)/$(OUTPUT_LIB) $(OBJ)

	cat $(DEPS) > $(R_DIR)/$(HEADER_NAME)
	sed -i '/#include "/d' $(R_DIR)/$(HEADER_NAME)
	
# example only
$(EX_NAME): $(R_DIR)/$(EX_NAME).cpp lib 
	$(CC) $(CFLAGS) -c -o $(OBJ_EXAMPLE) $(R_DIR)/$@.cpp
	$(CC) $(CFLAGS) -o $(R_DIR)/$@ $(OBJ_EXAMPLE) $(RELEASE_LIBS)

################
# object files #
################

$(O_DIR)/%.o: $(E_DIR)/%.cpp $(DEPS) | $(O_DIR)_exists
	$(CC) $(CFLAGS) -c -o $@ $<

$(O_DIR)/%.o: $(S_DIR)/%.cpp $(DEPS) | $(O_DIR)_exists
	$(CC) $(CFLAGS) -c -o $@ $<

$(O_DIR)_exists:
	mkdir -p $(O_DIR)

###########
# testing #
###########

test: $(PROJECT_NAME)
	$(R_DIR)/$(PROJECT_NAME) -m instances/testing/maps -s instances/testing/scenarios/test6.scen -e monosat_pebble_soc_all -t 1000 -p -a 8 -c formula.cnf -l 1 -f results.res -d 24

valgrind: $(PROJECT_NAME)
	valgrind --leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--verbose \
	--log-file=valgrind-out.txt \
	$(R_DIR)/$(PROJECT_NAME) -m instances/maps -s instances/scenarios/empty08-1.scen -e shift_parallel_mks_all -t 60 -p -a 5 -i 5 -l 1 -f results.res

test_example: $(EX_NAME)
	$(R_DIR)/$^

##############
# experiment #
##############

experiment: $(PROJECT_NAME)
	sh experiment.sh

#########
# clean #
#########

clean:
	rm -rf $(O_DIR)
	rm -rf $(R_DIR)/$(L_DIR)
	rm -f $(R_DIR)/$(PROJECT_NAME) $(R_DIR)/$(EX_NAME) $(R_DIR)/$(HEADER_NAME)
	rm -f valgrind-out.txt log.log *.cnf