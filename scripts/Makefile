# This makefile is here to simplify the automatic releases (and tests!!!)
# of the scripts

TS?=$(shell date '+%Y%m%d-%H%M')
RELEASEDIR=/export/ws06osmt/bin/scripts-$(TS)


VALID_TRAINING_SCRIPTS_NAMES=filter-model-given-input.pl  mert-moses.pl train-factored-phrase-model.perl
# Make trick to add directory name to all of them:
VALID_TRAINING_SCRIPTS=$(VALID_TRAINING_SCRIPTS_NAMES:%=training/%)

# the list of all scripts that should be released
VALID_SCRIPTS= $(VALID_TRAINING_SCRIPTS)

release:
	if [ -e $(RELEASEDIR) ]; then echo "Targetdir exists! Not touching it! $(RELEASEDIR)"; exit 1; fi
	mkdir -p $(RELEASEDIR)
	cp $(VALID_SCRIPTS) $(RELEASEDIR)
