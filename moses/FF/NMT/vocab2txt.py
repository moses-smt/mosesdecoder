import sys
import cPickle

d = cPickle.load(open(sys.argv[1], 'r'))
for w in d:
    print d[w]
