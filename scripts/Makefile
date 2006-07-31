# This makefile is here to simplify the automatic releases (and tests!!!)
# of the scripts


TS?=$(shell date '+%Y%m%d-%H%M')
DS?=$(shell date '+%Y%m%d')

### "MAIN" scripts are scripts that have a Philipp-like name, too
## That is for each script (listed below in MAIN_SCRIPTS),
## we create a date-stamped version in MAIN_SCRIPTS_TARGET_DIR

MAIN_SCRIPTS_TARGET_DIR=/export/ws06osmt/bin
# MAIN_SCRIPTS_TARGET_DIR=$(shell echo `pwd`/temp)

RELEASEDIR=/export/ws06osmt/bin/scripts-$(TS)
# RELEASEDIR=$(shell echo `pwd`/temp)




MAIN_TRAINING_SCRIPTS_NAMES=filter-model-given-input.pl  mert-moses.pl train-factored-phrase-model.perl
# Make trick to add directory name to all of them:
MAIN_TRAINING_SCRIPTS=$(MAIN_TRAINING_SCRIPTS_NAMES:%=training/%)

# the list of all scripts that should be released
MAIN_SCRIPTS= $(MAIN_TRAINING_SCRIPTS)

release:
	if [ -e $(RELEASEDIR) ]; then echo "Targetdir exists! Not touching it! $(RELEASEDIR)"; exit 1; fi
	mkdir -p $(RELEASEDIR)
	rsync -r --files-from ./released-files . $(RELEASEDIR)/
	## Now adding JHU-specific stuff
	mkdir -p $(RELEASEDIR)/extra/
	cp -r /export/ws06osmt/bin/cmert-0.5 $(RELEASEDIR)/extra/
	## And for each script, create/rewrite the daily release
	for s in $(MAIN_SCRIPTS); do \
	  bn=`basename $$s`; \
	  echo "export SCRIPTS_ROOTDIR=$(RELEASEDIR); $(RELEASEDIR)/$$s" > $(MAIN_SCRIPTS_TARGET_DIR)/$$bn-$(DS) || exit 1; \
	  chmod 775 $(MAIN_SCRIPTS_TARGET_DIR)/$$bn-$(DS); \
	done
	## URGE yourself to:
	@echo "   export SCRIPTS_ROOTDIR=$(RELEASEDIR)"
	## Release succeeded, tag the CVS
	cvs tag SCRIPTS-RELEASE-$(TS)
	

