import sys
import cPickle
import operator

d = cPickle.load(open(sys.argv[1], 'r'))
sorted_d = sorted(d.items(), key=operator.itemgetter(1))
for p in sorted_d:
    print p[0]
