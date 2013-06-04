CC = pclc.py
SOURCES = src_trg_tokeniser.pcl translation_model_training.pcl
OBJS = $(SOURCES:.pcl=.py)

all: build

build: $(OBJS)

%.py: %.pcl
	$(CC) $<

clean:
	rm -f *.py *.pyc *.log *~
