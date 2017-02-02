#!/usr/bin/env python

# Quick hack to extract lexica from Giza-Aligned corpus
# (c) 2011 Ulrich Germann

import sys, os

D = os.popen("zcat %s" % sys.argv[1])
E = os.popen("zcat %s" % sys.argv[2])
A = os.popen("zcat %s" % sys.argv[3])
d_given_e = sys.argv[4]
e_given_d = sys.argv[5]

try:
    os.makedirs(os.path.dirname(d_given_e))
    os.makedirs(os.path.dirname(e_given_d))
except:
    pass

WD = ["NULL","UNK"]
WE = ["NULL","UNK"]
VD = {}
VE = {}
JJ = []
MD = []
ME = []

def id(V,W,x):
    i =  V.setdefault(x,len(W))
    if i == len(W): W.append(x)
    return i

ctr = 0
for dline in D:
    ctr += 1
    #if ctr % 1000 == 0: sys.stderr.write('.')
    eline = E.readline()
    aline = A.readline()
    d = [id(VD,WD,w) for w in dline.strip().split()]
    e = [id(VE,WE,w) for w in eline.strip().split()]
    a = [[int(y) for y in x.split('-')] for x in aline.split()]

    while len(MD) <= len(VD) + 2:
        MD.append(0)
        JJ.append({})
        pass

    while len(ME) <= len(VE) + 2:
        ME.append(0)
        pass
    
    fd = [0 for i in xrange(len(d))]
    fe = [0 for i in xrange(len(e))]
    for x,y in a:
        fd[x]         += 1
        fe[y]         += 1
        MD[d[x]]      += 1
        ME[e[y]]      += 1
        JJ[d[x]][e[y]] = JJ[d[x]].setdefault(e[y],0) + 1
        # print WD[d[x]],WE[e[y]],JJ[d[x]][e[y]]
        pass
    for i in [d[k] for k in xrange(len(d)) if fd[k] == 0]:
        ME[0]   += 1
        MD[i]   += 1
        JJ[i][0] = JJ[i].setdefault(0,0) + 1
        pass
    for i in [e[k] for k in xrange(len(e)) if fe[k] == 0]:
        ME[i]   += 1
        MD[0]   += 1
        JJ[0][i] = JJ[0].setdefault(i,0) + 1
        pass
    pass

ED = os.popen("gzip > %s" % e_given_d, 'w')
DE = os.popen("gzip > %s" % d_given_e, 'w')

for d in xrange(len(JJ)):
    T = JJ[d]
    for e,jj in T.items():
        print >>ED, WE[e], WD[d], float(jj)/MD[d]
        print >>DE, WD[d], WE[e], float(jj)/ME[e]
        pass
    pass

ED.close()
DE.close()
