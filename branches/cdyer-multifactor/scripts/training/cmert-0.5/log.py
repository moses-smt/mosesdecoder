#!/usr/bin/python2.3
import sys

level = 1
file = sys.stderr

def writeln(s=""):
    file.write("%s\n" % s)
    file.flush()

def write(s):
    file.write(s)
    file.flush()


    
    
