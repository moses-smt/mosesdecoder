# This makefile is here to simplify the automatic releases (and tests!!!)
# of the scripts


TS?=$(shell date '+%Y%m%d-%H%M')
DS?=$(shell date '+%Y%m%d')

# to simplify redirect to custom releases
DEFAULTTARGETDIR?=/export/ws06osmt/bin

MAIN_SCRIPTS_TARGET_DIR=$(TARGETDIR)
# MAIN_SCRIPTS_TARGET_DIR=$(shell echo `pwd`/temp)

RELEASEDIR=$(TARGETDIR)/scripts-$(TS)
# RELEASEDIR=$(shell echo `pwd`/temp)



## Rules to compile parts that need compilation


all: compile

compile:
	cd training/cmert-0.5/ && make
	## Compiled mert
	cd training/phrase-extract/ && make
	## Compiled phrase_extract
	cd training/symal/ && make
	## Compiled symal
	## All files that need compilation were compiled


### "MAIN" scripts are scripts that have a Philipp-like name, too
## That is for each script (listed below in MAIN_SCRIPTS),
## we create a date-stamped version in MAIN_SCRIPTS_TARGET_DIR

MAIN_TRAINING_SCRIPTS_NAMES=filter-model-given-input.pl  mert-moses.pl train-factored-phrase-model.perl clean-corpus-n.perl
# Make trick to add directory name to all of them:
MAIN_TRAINING_SCRIPTS=$(MAIN_TRAINING_SCRIPTS_NAMES:%=training/%)

MAIN_GENERIC_SCRIPTS_NAMES= moses-parallel.pl
# Make trick to add directory name to all of them:
MAIN_GENERIC_SCRIPTS=$(MAIN_GENERIC_SCRIPTS_NAMES:%=generic/%)

# the list of all scripts that should be released
MAIN_SCRIPTS= $(MAIN_TRAINING_SCRIPTS) $(MAIN_GENERIC_SCRIPTS)


release:
	# Compile the parts
	$(MAKE) all
	@./check-dependencies.pl "$(HOME)" "$(TARGETDIR)" "$(RELEASEDIR)" "$(BINDIR)"
	mkdir -p $(RELEASEDIR)
	rsync -r --files-from ./released-files . $(RELEASEDIR)/
	sed 's#^my \$$BINDIR\s*=.*#my \$$BINDIR="$(BINDIR)";#' training/train-factored-phrase-model.perl > $(RELEASEDIR)/training/train-factored-phrase-model.perl
	@echo "####### Do not forget to:" >> $(RELEASEDIR)/README
	@echo "  export SCRIPTS_ROOTDIR=$(RELEASEDIR)" >> $(RELEASEDIR)/README
	## Remember, only files listed in released-files are released!!
	## Don't forget to set your SCRIPTS_ROOTDIR with:
	@echo "   export SCRIPTS_ROOTDIR=$(RELEASEDIR)"

generate_wrappers:
	## And for each script, create/rewrite the daily release
	export TARGET
	@for s in $(MAIN_SCRIPTS); do \
	  bn=`basename $$s`; \
	  echo '#!/bin/bash' > $(MAIN_SCRIPTS_TARGET_DIR)/$$bn-$(DS) || exit 1; \
	  echo "export SCRIPTS_ROOTDIR=$(RELEASEDIR); $(RELEASEDIR)/$$s "'"$$@"; exit $$?' >> $(MAIN_SCRIPTS_TARGET_DIR)/$$bn-$(DS) || exit 1; \
	  chmod 775 $(MAIN_SCRIPTS_TARGET_DIR)/$$bn-$(DS); \
	done
	

MOSESRELEASE=$(TARGETDIR)/moses.$(DS)
## This is a handy goal to release moses binary, too
releasemoses:
	if [ -z "$(TARGETDIR)" ]; then \
	  echo "Please specify a TARGETDIR." ; \
	  echo "  For custom releases issue: "; \
	  echo "   TARGETDIR=$(HOME)/releases make releasemoses"; \
	  echo "  For official releases: "; \
	  echo "   TARGETDIR=/export/ws06osmt make releasemoses"; \
	  exit 1; \
	fi
	if [ -e $(MOSESRELEASE) ]; then echo "Moses release exists! Not touching it! $(MOSESRELEASE)"; exit 1; fi
	if [ ! -e ../moses-cmd/src/moses ]; then echo "Moses (../moses-cmd/src/moses) does not exist, nothing to release"; ecit 1; fi
	if file ../moses-cmd/src/moses | grep -q 'dynamicall' ; then echo "Moses (../moses-cmd/src/moses) is dynamically linked, not releasing."; ecit 1; fi
	cp ../moses-cmd/src/moses $(MOSESRELEASE)
	## Your current version of moses:
	@echo "   $(MOSESRELEASE)"


## This goal lists all files you might have wanted to release
# but forgot to mention in released-files
missed:
	### These might be intended for release
	find . -type f \
	| grep -v '/CVS/' \
	| grep -v /tests/ \
	| sed 's/^\.\///' \
	| grep -F -x -v -f released-files


### Tests, applicable only at JHU environment due to data dependencies
export WORKSPACE=$(shell pwd)/../

.PHONY: tests
tests:
	export SCRIPTS_ROOTDIR=`pwd`; \
	cd tests; \
	ts=`date '+%Y%m%d-%H%M%N'`; \
	for test in *.test; do  \
	  mkdir $$test.$$ts; \
	  cd $$test.$$ts; \
	  echo "Running $$test  in  tests/$$test.$$ts"; \
	  ../$$test > log 2>&1 || exit 1; \
	  cd ..; \
	done
	## All tests passed

## Run just one test
tests/%.test.run: tests/%.test
	export SCRIPTS_ROOTDIR=`pwd`; \
	ts=`date '+%Y%m%d-%H%M%N'`; \
	cd tests; \
	  test=$*.test; \
	  mkdir $$test.$$ts; \
	  cd $$test.$$ts; \
	  echo "Running $$test  in  tests/$$test.$$ts"; \
	  ( nohup ../$$test > log 2>&1 & ) || exit 1; \
	  echo "Observe tests/$$test.$$ts/log"; \
	cd ..
	
