#!/usr/bin/python

# $Id: log.py 1307 2007-03-14 22:22:36Z hieuhoang1972 $
import sys

level = 1
file = sys.stderr

def writeln(s=""):
    file.write("%s\n" % s)
    file.flush()

def write(s):
    file.write(s)
    file.flush()


    
    
