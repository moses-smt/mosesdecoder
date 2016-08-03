

import numpy
import sys

for name in sys.argv[1:]:
    lens = []
    with open(name, 'r') as f:
        for ll in f:
            lens.append(len(ll.strip().split(' ')))
    print name, ' max ', numpy.max(lens), ' min ', numpy.min(lens), ' average ', numpy.mean(lens)



