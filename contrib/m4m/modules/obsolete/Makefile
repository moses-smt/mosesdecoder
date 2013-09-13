# -*- Makefile -*-
# Mandatory at the beginning of the file, before m4m inclusions

# L1,L2: tags that identify translation source (L1) 
#        and translation target (L2) language
L1 ?= de
L2 ?= en

LL = $(word 1, $(sort ${L1} ${L2}))-$(word 2, $(sort ${L1} ${L2}))
# a name for this experiment
experiment = dynsa-vs-std-phrase-table

# the working directry
WDIR = $(CURDIR)
MOSES_ROOT = ${HOME}/code/moses/master/mosesdecoder

# include m4m boilerplate
include ${MOSES_ROOT}/contrib/m4m/modules/m4m.m4m

$(info M4MDIR=${m4mdir})

#include ${m4mdir}/baseline-system.make
#include ${m4mdir}dynsa-system.make
#$(info ${MY_EXPERIMENT})

tune.sets = $(subst /raw/,/cased/,$(wildcard crp/dev/raw/*.${L1}.gz))

all:
.PHONY: all

ifdef tune.runs
$(foreach tuneset, $(word 1,${tune.sets:.${L1}.gz=}),\
$(foreach run,$(shell seq ${tune.runs}),\
  $(eval $(call tune_system,baseline/moses.ini.0,\
  baseline/tuned/$(notdir ${tuneset})/${run}/moses.ini,\
  ${tuneset}.${L1},${tuneset}.${L2},0));\
  $(if ,$(info $(call tune_system,baseline/moses.ini.0,\
  baseline/tuned/$(notdir ${tuneset})/${run}/moses.ini,\
  ${tuneset}.${L1},${tuneset}.${L2},0));)\
  $(eval $(call copy_weights,dynsa/moses.ini.0,\
  baseline/tuned/$(notdir ${tuneset})/${run}/moses.ini,\
  dynsa/tuned/$(notdir ${tuneset})/${run}/moses.ini));\
  $(if ,$(info $(call copy_weights,dynsa/moses.ini.0,\
  baseline/tuned/$(notdir ${tuneset})/${run}/moses.ini,\
  dynsa/tuned/$(notdir ${tuneset})/${run}/moses.ini));)\
  $(foreach evalset,$(word 2,${tune.sets:.${L1}.gz=}),\
  $(foreach system,baseline dynsa,\
    $(eval evaltarget:=${system}/eval/$(notdir ${tuneset})/${run}/$(notdir ${evalset}));\
    $(eval $(call bleu_eval,${evaltarget},\
    ${system}/tuned/$(notdir ${tuneset})/${run}/moses.ini,\
    ${evalset}.${L1},${moses.inputtype.plaintext},${evalset}.${L2}));\
    $(if ,$(info $(call bleu_eval,${evaltarget},\
    ${system}/tuned/$(notdir ${tuneset})/${run}/moses.ini,\
    ${evalset}.${L1},${moses.inputtype.plaintext},${evalset}.${L2}));)\
    ));\
))

all: ${EVALUATIONS}
	echo EVALS ${EVALUATIONS}
else
all:
	$(foreach n,$(shell seq 1 1),${MAKE} tune.runs="$n $n";)
endif

