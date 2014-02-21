SUBDIRS = tokenizer

all: subdirs

clean:
	for dir in $(SUBDIRS); do \
    $(MAKE) -C $$dir clean; \
  done

subdirs:
	for dir in $(SUBDIRS); do \
    $(MAKE) -C $$dir ; \
  done

