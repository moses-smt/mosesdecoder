#!/usr/bin/env python3

"""Reduces an ngrams file for training nplm to a smaller version of it with less ngrams"""
from sys import argv

if len(argv) != 5:
    print("Wrong number of args, got: " + str(len(argv) - 1) + " expected 4.")
    print("Usage: reduce_ngrams.py INFILE OUTFILE START_IDX NGRAMS")
    exit()

INFILE = open(argv[1], 'r')
OUTFILE = open(argv[2], 'w')
START_IDX = int(argv[3])
NGRAMS = int(argv[4])

for line in INFILE:
    line = line.split()
    line = line[START_IDX:START_IDX+NGRAMS]
    linetowrite = ""
    for token in line:
        linetowrite = linetowrite + token + " "
    #Strip final empty space and add newline
    linetowrite = linetowrite[:-1]
    linetowrite = linetowrite + '\n'
    OUTFILE.write(linetowrite)

INFILE.close()
OUTFILE.close()
