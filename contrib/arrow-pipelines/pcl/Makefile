CC = pclc.py
CFLAGS=-i
SOURCES = training_pipeline.pcl
OBJS = $(SOURCES:.pcl=.py)
SUBDIRS = components

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
