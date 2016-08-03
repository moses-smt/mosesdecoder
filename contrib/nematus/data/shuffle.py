import os
import sys
import random

from tempfile import mkstemp
from subprocess import call



def main(files):

    tf_os, tpath = mkstemp()
    tf = open(tpath, 'w')

    fds = [open(ff) for ff in files]

    for l in fds[0]:
        lines = [l.strip()] + [ff.readline().strip() for ff in fds[1:]]
        print >>tf, "|||".join(lines)

    [ff.close() for ff in fds]
    tf.close()

    tf = open(tpath, 'r')
    lines = tf.readlines()
    random.shuffle(lines)

    fds = [open(ff+'.shuf','w') for ff in files]

    for l in lines:
        s = l.strip().split('|||')
        for ii, fd in enumerate(fds):
            print >>fd, s[ii]

    [ff.close() for ff in fds]

    os.remove(tpath)

if __name__ == '__main__':
    main(sys.argv[1:])

    


