#!/usr/bin/env python
# Auxiliary script to convert fast_align output to the "bal" input format
# that symal requires.
# Script by Ulrich Germann.

# command line args: 
#   <L1 plain text> <L2 plain text> <L1-L2 alignments> <L2-L1 alignments>
#
# TO DO: - proper argument parsing with getopt
#        - help text

import sys,os

(T1,T2,fwd,bwd) = [open(x) for x in sys.argv[1:]]

def alnvec(slen,alinks,mode):
    d = dict([[int(x[mode]),int(x[(mode+1)%2])+1] for x 
              in [y.split('-') for y in alinks]])
    return [d.get(i,0) for i in xrange(slen)]

ctr = 0
for t1 in T1:
    t1 = t1.strip().split()
    t2 = T2.readline().strip().split()
    a1 = alnvec(len(t1),bwd.readline().split(),0)
    a2 = alnvec(len(t2),fwd.readline().split(),1)
    print 1
    print len(t2), " ".join(t2), '#', " ".join(["%d"%x for x in a2])
    print len(t1), " ".join(t1), '#', " ".join(["%d"%x for x in a1])
    ctr += 1
    pass
