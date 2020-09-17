#CC = /home/jykang/Xilinx/SDK/2019.1/gnu/aarch64/lin/aarch64-linux/bin/aarch64-linux-gnu-g++
CC = /tools/Xilinx/SDK/2018.3/gnu/aarch64/lin/aarch64-linux/bin/aarch64-linux-gnu-g++

INCLUDES  := 

#LIBRARIES := /home/jykang/Xilinx/SDK/2019.1/gnu/aarch64/lin/aarch64-linux/aarch64-linux-gnu/libc/lib
LIBRARIES := /tools/Xilinx/SDK/2018.3/gnu/aarch64/lin/aarch64-linux/aarch64-linux-gnu/libc/lib

LIBS =
CCFLAGS = -O3
CXXFLAGS = -std=c++11

ifeq ($(NDP_COMPILE), 1)
CCFLAGS += -march=armv8-a+simd -static
LIBS += -lgomp
endif

################################################################################
# Source file directory:
SRC_DIR = src
# Object file directory:
OBJ_DIR = bin
# Include header file diretory:
INC_DIR = include

## Make variables ##
# Target executable name:
EXE = main

# Object files:
#ifeq ($(NDP_COMPILE), 1)
OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/backward_simd.o $(OBJ_DIR)/forward_simd.o $(OBJ_DIR)/ndp_sls.o
#else 
#OBJS = $(OBJ_DIR)/main.o
#endif

##########################################################
## Compile ##

all : $(EXE)

test: test_embedding.c
	$(CC) $< -o test_embedding
test2: test.c
	gcc $< -o run

# Link c++ compiled object files to target executable:
$(EXE) : $(OBJS)
	$(EXEC) $(CC) $(CCFLAGS) -o $@ $+ -L$(LIBRARIES) $(LIBS)

# Compile main.c file to object files:
$(OBJ_DIR)/%.o : %.c
	$(EXEC) $(CC) $(CCFLAGS) -c $< -o $@

# Compile C++ source files to object files:
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp $(INC_DIR)/%.h
	$(CC) $(CCFLAGS) $(CXXFLAGS) -c $< -o $@ -L$(LIBRARIES) $(LIBS)

# Clean objects in object directory.
clean:
	$(RM) bin/* *.o $(EXE) run test_embedding

