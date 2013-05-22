CC = pclc.py
SOURCES = training_pipeline.pcl
OBJS = $(SOURCES:.pcl=.py)
SUBDIRS = components

all: subdirs build

build: $(OBJS)

%.py: %.pcl
	$(CC) $<

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f *.py *.pyc *.log *~

subdirs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir ; \
	done
