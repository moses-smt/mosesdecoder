CC = pclc.py
CFLAGS = -i
SOURCES = src_trg_tokeniser.pcl translation_model_training.pcl
OBJS = $(SOURCES:.pcl=.py)
SUBDIRS = wrappers

all: subdirs build

build: $(OBJS)

%.py: %.pcl
	$(CC) $(CFLAGS) $<

clean:
	for dir in $(SUBDIRS); do \
    $(MAKE) -C $$dir clean; \
  done
	rm -f *.py *.pyc *.log *~

subdirs:
	for dir in $(SUBDIRS); do \
    $(MAKE) -C $$dir ; \
  done

