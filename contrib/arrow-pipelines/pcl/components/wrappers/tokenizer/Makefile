CC = pclc.py
CFLAGS = -i
SOURCES = tokenizer.pcl
OBJS = $(SOURCES:.pcl=.py)

all: build

build: $(OBJS)

%.py: %.pcl
	$(CC) $(CFLAGS) $<

clean:
	rm -f *.py *.pyc *.log *~

