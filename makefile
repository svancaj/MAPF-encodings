####################
# define variables #
####################

CC = g++
CFLAGS = -std=c++11 -O3 -Wall -Wextra -pedantic
S_DIR = src
E_DIR = $(S_DIR)/encodings
EX_DIR = $(S_DIR)/externals
O_DIR = .object_files
L_DIR = libs
R_DIR = release
PROJECT_NAME = MAPF
OUTPUT_LIB = libmapf.a
HEADER_NAME = MAPF.hpp
EX_NAME = example

#_SHARED_LIBS = z gmpxx gmp
#SHARED_LIBS = $(patsubst %,-l%,$(_SHARED_LIBS))
_LIBS = libpb.a libkissat.a #libmonosat.a
LIBS = $(patsubst %,$(L_DIR)/%,$(_LIBS))
RELEASE_LIBS = $(patsubst %,$(R_DIR)/$(L_DIR)/%,$(OUTPUT_LIB)) $(patsubst %,$(R_DIR)/$(L_DIR)/%,$(_LIBS))

_DEPS = instance.hpp logger.hpp encodings/solver_common.hpp
DEPS = $(patsubst %,$(S_DIR)/%,$(_DEPS))

VAR = at pass shift #monosat-pass monosat-shift
MOVE = parallel pebble
FUNC = mks soc
COMP = all lazy

_ENC_OBJ = solver_common.o monosat-pass_parallel_mks_all.o monosat-pass_parallel_soc_all.o monosat-shift_parallel_mks_all.o monosat-shift_parallel_soc_all.o $(foreach v, $(VAR), $(foreach m, $(MOVE), $(foreach f, $(FUNC), $(foreach c, $(COMP), $v_$m_$f_$c.o)))) 
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
#	$(CC) $(CFLAGS) -o $(R_DIR)/$@ $^ $(LIBS) $(SHARED_LIBS)

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
#	$(CC) $(CFLAGS) -o $(R_DIR)/$@ $(OBJ_EXAMPLE) $(RELEASE_LIBS) $(SHARED_LIBS)

################
# object files #
################

$(O_DIR)/%.o: $(E_DIR)/%.cpp $(DEPS) | $(O_DIR)_exists
	$(CC) $(CFLAGS) -I $(EX_DIR) -c -o $@ $<

$(O_DIR)/%.o: $(S_DIR)/%.cpp $(DEPS) | $(O_DIR)_exists
	$(CC) $(CFLAGS) -I $(EX_DIR) -c -o $@ $<

$(O_DIR)_exists:
	mkdir -p $(O_DIR)

###########
# testing #
###########

test: $(PROJECT_NAME)
	$(R_DIR)/$(PROJECT_NAME) -m instances/testing/maps -s instances/testing/scenarios/test2.scen -e monosat-shift_parallel_mks_all -t 100 -a 2 -l 2 -c tmp.cnf

valgrind: $(PROJECT_NAME)
	valgrind --leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--verbose \
	--log-file=valgrind-out.txt \
	$(R_DIR)/$(PROJECT_NAME) -m instances/maps -s instances/scenarios/random_10_1.scen -e monosat-shift_parallel_mks_all -t 100000 -a 10 -l 1 -c tmp.cnf -f results.res

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
	rm -f valgrind-out.txt log.log *.cnf tmp*