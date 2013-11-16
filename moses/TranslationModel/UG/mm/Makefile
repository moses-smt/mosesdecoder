# Some systems apparently distinguish between shell 
# variables and environment variables. The latter are
# visible to the make utility, the former apparently not,
# so we need to set them if they are not defined yet

# ===============================================================================
# COMPILATION PREFERENCES
# ===============================================================================
# CCACHE: if set to ccache, use ccache to speed up compilation
# OPTI:   optimization level
# PROF:   profiler switches 

CCACHE  = ccache
OPTI    = 3
EXE_TAG = exe
PROF = 
# PROF = -g -pg

# ===============================================================================

SHELL         = bash
MAKEFLAGS    += --warn-undefined-variables
.DEFAULT_GOAL = all
.SUFFIXES:

# ===============================================================================
# COMPILATION 'LOCALIZATION'
HOST     ?= $(shell hostname)
HOSTTYPE ?= $(shell uname -m)

MOSES_ROOT  = ${HOME}/code/moses/master/mosesdecoder
WDIR        = build/${HOSTTYPE}/${OPTI}
VPATH       = ${HOME}/code/moses/master/mosesdecoder/
CXXFLAGS    = ${PROF} -ggdb -Wall -O${OPTI} ${INCLUDES} 
CXXFLAGS   += -DMAX_NUM_FACTORS=4
CXXFLAGS   += -DKENLM_MAX_ORDER=5
modirs     := $(addprefix -I,$(shell find ${MOSES_ROOT}/moses ${MOSES_ROOT}/contrib -type d))
CXXFLAGS   += -I${MOSES_ROOT} 
INCLUDES    = 
BZLIB       =  
BOOSTLIBTAG = 

REQLIBS = m z pthread lzma ${BZLIB} \
	boost_thread${BOOSTLIBTAG} \
	boost_iostreams${BOOSTLIBTAG} \
	boost_program_options${BOOSTLIBTAG} \
	boost_system${BOOSTLIBTAG} \
	boost_filesystem${BOOSTLIBTAG} 

# 	icuuc icuio icui18n \

LIBS     = $(addprefix -l, ${REQLIBS} moses) 
LIBDIRS  = -L${HOME}/code/moses/master/mosesdecoder/lib
BINDIR = bin
ifeq "$(OPTI)" "0"
BINPREF = debug.
else
BINPREF = 
endif


OBJ2 :=

define compile 

DEP  += ${WDIR}/$(basename $(notdir $1)).d
${WDIR}/$(basename $(notdir $1)).o : $1 $(wildcard $(basename $1).h)
	@echo -e "COMPILING $1"
	@mkdir -p $$(@D)
	${CXX} ${CXXFLAGS} -MD -MP -c $$(abspath $$<) -o $$@

endef

programs = mtt-build mtt-dump symam2mam custom-pt mmlex-build

all: $(addprefix ${BINDIR}/${BINPREF}, $(programs))
	@echo $^
clean:
	rm -f ${WDIR}/*.o ${WDIR}/*.d

custom-pt: ${BINDIR}/${BINPREF}custom-pt
	echo $^

INMOGEN = $(wildcard ${MOSES_ROOT}/moses/generic/*/*.cpp)
OBJ     = $(patsubst %.cc,%.o,$(wildcard $(patsubst %.h,%.cc,$(wildcard *.h))))
OBJ    += $(patsubst %.cpp,%.o,${INMOGEN})
EXE     = $(patsubst %.cc,%.o,$(filter-out $(patsubst %.h,%.cc,$(wildcard *.h)),$(wildcard *.cc)))

$(foreach cpp,${INMOGEN},$(eval $(call compile,${cpp})))
$(foreach cpp,$(wildcard *.cc),$(eval $(call compile,${cpp})))
$(addprefix ${BINDIR}/${BINPREF}, $(programs)): $(addprefix ${WDIR}/,$(notdir ${OBJ}))
$(addprefix ${BINDIR}/${BINPREF}, $(programs)): ${MOSES_ROOT}/lib/libmoses.a 
${BINDIR}/${BINPREF}%: ${WDIR}/%.o  
	echo PREREQS: $<
	$(CXX) $(CXXFLAGS) -o $@ $^ ${LIBDIRS} ${LIBS} 

.SECONDARY: 

-include $(DEP)

