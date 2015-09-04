SHELL = bash
MAKEFLAGS += --warn-undefined-variables
.DEFAULT_GOAL = all
.SUFFIXES:

# ===============================================================================
# LOCAL ENVIRONMENT
# ===============================================================================

# shell script snippet:
define find_moses_root
d=$$(pwd);
while [[ ! -e $$d/Jamroot && $$d != "/" ]] ; do
    d=$$(dirname $$d);
done;
echo $$d
endef

MOSES_ROOT := $(shell $(find_moses_root))

# ===============================================================================
# COMPILATION PREFERENCES
# ===============================================================================
# CCACHE: if set to ccache, use ccache to speed up compilation
# OPTI:   optimization level
# PROF:   profiler switches

# compiler command
compiler ?= g++
variant  ?= runtime
link     ?= static

CXX       = $(shell which ccache) $(compiler)
CXXFLAGS += -DMAX_NUM_FACTORS=4
CXXFLAGS += -DKENLM_MAX_ORDER=5
CXXFLAGS += -DWITH_THREADS
CXXFLAGS += -DNO_MOSES
CXXFLAGS += -I${MOSES_ROOT} -I.

ifeq ($(variant),debug)
CXXFLAGS += -ggdb -O0
else ifeq ($(variant),profile)
CXXFLAGS += -g -pg -O3
else ifeq ($(variant),syntax)
CXXFLAGS += -fsyntax-only
endif

# LDFLAGS  = -L${MOSES_ROOT}/lib -L ./lib/

# WDIR = build/$(variant)/${HOSTTYPE}/${KERNEL}
WDIR = build/$(variant)

# ===============================================================================
# COMPILATION 'LOCALIZATION'
HOST       ?= $(shell hostname)
HOSTTYPE   ?= $(shell uname -m)
KERNEL      = $(shell uname -r)

nil:

# libraries required

LIBS  = m z bz2 pthread dl ${BOOSTLIBS} 
#LIBS += tcmalloc
BOOSTLIBS := thread system filesystem program_options iostreams
BOOSTLIBS := $(addprefix boost_,${BOOSTLIBS})
ifdef ($(BOOSTLIBTAG),"")
BOOSTLIBS := $(addsuffix ${BOOSTLIBTAG},${BOOSTLIBS})
endif

cc2obj = $(addsuffix .o,$(patsubst ${MOSES_ROOT}%,$(WDIR)%,\
	 	        $(patsubst .%,$(WDIR)%,$(basename $1))))
cc2exe = $(addprefix ./bin/$(variant)/,$(basename $(notdir $1)))
cc2trg = $(basename $(notdir $1))

define compile

DEP += $(basename $(call cc2obj,$1)).d
$(call cc2obj,$1): $1
	@echo -e "COMPILING $1"
	@mkdir -p $$(@D)
	@${CXX} ${CXXFLAGS} -MD -MP -c $$< -o $$@

endef

define build

$(call cc2trg,$1): $(call cc2exe,$1)
$(call cc2exe,$1): $(call cc2obj,$1) $(LIBOBJ)
ifneq ($(variant),syntax)
	@echo -e "LINKING $$@"
	@mkdir -p $${@D}
	@${CXX} ${CXXFLAGS} -o $$@ $(LIBOBJ) $(addprefix -l,${LIBS}) $$<
endif

endef

# list files here that you want explicitly excluded from compilation
skip  = sim-pe.cc
skip += mtt.count.cc
skip += try-align2.cc
skip += spe-check-coverage3.cc
skip += mmsapt.cpp
skip += ug_stringdist.cc
skip += ug_splice_arglist.cc
# skip += ug_lexical_reordering.cc

# objects from elsewhere in the moses tree that are needed
extra = ${MOSES_ROOT}/util/exception.cc

$(foreach f,$(skip),$(eval broken+=$(shell find -name $f)))
broken += $(wildcard ./mm/stashed/*)

$(info SCANNING DIRECTORY TREE FOR FILES)
find_cfiles = find -name '*.cc' -or -name '*.cpp'
CFILES    = $(filter-out $(broken), $(shell $(find_cfiles)))
PROGRAMS := $(shell $(find_cfiles) | xargs grep -lP '^(int +)?main')
PROGRAMS := $(filter-out $(broken),$(PROGRAMS))

ALLOBJ = $(call cc2obj,$(CFILES) $(extra))
LIBOBJ = $(call cc2obj,$(filter-out $(PROGRAMS),$(CFILES) $(extra)))

$(foreach f,$(CFILES) $(extra),$(eval $(call compile,$f)))
$(foreach p,$(PROGRAMS),$(eval $(call build,$p)))

ifeq ($(filter relink,$(MAKECMDGOALS)),relink)
.PHONY: relink
$(foreach p,$(PROGRAMS),$(eval .PHONY: $(call cc2exe,$p)))
relink: $(filter-out relink,$(MAKECMDGOALS))
endif

-include $(DEP)

