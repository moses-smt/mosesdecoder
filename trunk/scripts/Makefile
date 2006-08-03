# This makefile is here to simplify the automatic releases (and tests!!!)
# of the scripts


TS?=$(shell date '+%Y%m%d-%H%M')
DS?=$(shell date '+%Y%m%d')

### "MAIN" scripts are scripts that have a Philipp-like name, too
## That is for each script (listed below in MAIN_SCRIPTS),
## we create a date-stamped version in MAIN_SCRIPTS_TARGET_DIR

# to simplify redirect to custom releases
DEFAULTTARGETDIR?=/export/ws06osmt/bin

MAIN_SCRIPTS_TARGET_DIR=$(TARGETDIR)
# MAIN_SCRIPTS_TARGET_DIR=$(shell echo `pwd`/temp)

RELEASEDIR=$(TARGETDIR)/scripts-$(TS)
# RELEASEDIR=$(shell echo `pwd`/temp)




MAIN_TRAINING_SCRIPTS_NAMES=filter-model-given-input.pl  mert-moses.pl train-factored-phrase-model.perl clean-corpus-n.perl
# Make trick to add directory name to all of them:
MAIN_TRAINING_SCRIPTS=$(MAIN_TRAINING_SCRIPTS_NAMES:%=training/%)

MAIN_GENERIC_SCRIPTS_NAMES= moses-parallel.pl
# Make trick to add directory name to all of them:
MAIN_GENERIC_SCRIPTS=$(MAIN_GENERIC_SCRIPTS_NAMES:%=generic/%)

# the list of all scripts that should be released
MAIN_SCRIPTS= $(MAIN_TRAINING_SCRIPTS) $(MAIN_GENERIC_SCRIPTS)

release:
	if [ -z "$(TARGETDIR)" ]; then echo "Specify TARGETDIR=/export/ws06osmt to do a real release" ; exit 1; fi
	if [ -e $(RELEASEDIR) ]; then echo "Targetdir exists! Not touching it! $(RELEASEDIR)"; exit 1; fi
	mkdir -p $(RELEASEDIR)
	rsync -r --files-from ./released-files . $(RELEASEDIR)/
	## Now adding JHU-specific stuff
	mkdir -p $(RELEASEDIR)/extra/
	cp -r /export/ws06osmt/bin/cmert-0.5 $(RELEASEDIR)/extra/
	## Back from JHU-specific
	echo "####### Do not forget to:" >> $(RELEASEDIR)/README
	echo "  export SCRIPTS_ROOTDIR=$(RELEASEDIR)" >> $(RELEASEDIR)/README
	## Remember, only files listed in released-files are released!!
	## URGE yourself to:
	@echo "   export SCRIPTS_ROOTDIR=$(RELEASEDIR)"

jhu_release:
	TS=$(TS) DS=$(DS) TARGETDIR=$(DEFAULTTARGETDIR) make release
	TS=$(TS) DS=$(DS) TARGETDIR=$(DEFAULTTARGETDIR) make generate_wrappers
	## Remember, only files listed in released-files are released!!
	### Release succeeded, tagging the CVS
	cvs tag SCRIPTS-RELEASE-$(TS)

generate_wrappers:
	## And for each script, create/rewrite the daily release
	export TARGET
	for s in $(MAIN_SCRIPTS); do \
	  bn=`basename $$s`; \
	  echo '#!/bin/bash' > $(MAIN_SCRIPTS_TARGET_DIR)/$$bn-$(DS) || exit 1; \
	  echo "export SCRIPTS_ROOTDIR=$(RELEASEDIR); $(RELEASEDIR)/$$s "'"$$@"; exit $$?' >> $(MAIN_SCRIPTS_TARGET_DIR)/$$bn-$(DS) || exit 1; \
	  chmod 775 $(MAIN_SCRIPTS_TARGET_DIR)/$$bn-$(DS); \
	done
	


## This goal lists all files you might have wanted to release
# but forgot to mention in released-files
missed:
	### These might be intended for release
	find . -type f \
	| grep -v '/CVS/' \
	| sed 's/^\.\///' \
	| grep -F -x -v -f released-files



### Tests, applicable only at JHU environment
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

