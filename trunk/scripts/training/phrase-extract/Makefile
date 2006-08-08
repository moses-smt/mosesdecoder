DECODER_FLAG	= -D_DECODER

# ================================================================
#LINUX VARS
CC  = /usr/bin/gcc# -Wno-deprecated
CXX = /usr/bin/g++# -Wno-deprecated
CCFLAGS = $(BYTESWAP_FLAG) $(DECODER_FLAG) -O3
BYTESWAP_FLAG   = -DSLM_SWAP_BYTES
OSNAME = linux

# ================================================================
# DIRECTORIES TO BE USED
# general variables
TopDir := $(shell pwd)
BinDir = $(TopDir)/bin
SrcDir = $(TopDir) $(TopDir)/extract
INCLUDES = -I$(TopDir) -I$(TopDir)/extract
ObjDir = $(TopDir)/extractobj.$(OSNAME)

# =========================================================
# Variables for the EXTRACTOR

ExtractObjFiles = extract.o

ExtractObjFilesWithPath = $(addprefix $(ObjDir)/, $(ExtractObjFiles))

# =========================================================

extract: OutputPaths $(ExtractObjFiles)
	$(CXX) $(CCFLAGS) $(INCLUDES) -o $(BinDir)/phrase-extract $(ExtractObjFilesWithPath)

OutputPaths: MakeObjDir MakeBinDir

MakeObjDir: 
	@mkdir -p $(ObjDir)
MakeBinDir:
	@mkdir -p $(BinDir)

VPATH = $(ObjDir) $(SrcDir)

%.o : %.cpp
	@echo @$(CXX) -c $(CCFLAGS) $(INCLUDES) $< -o $(ObjDir)/$(@F)
	@echo Compiling $(@F)
	@$(CXX) -c $(CCFLAGS) $(INCLUDES) $< -o $(ObjDir)/$(@F)
	@echo

%.o : %.c
	@echo Compiling $(@F)
	@$(CC) -c $(CCFLAGS) $(INCLUDES) $< -o $(ObjDir)/$(@F)
	@echo
