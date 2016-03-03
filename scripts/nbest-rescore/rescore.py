#!/usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

import sys

FEAT_FIELD = 2
SCORE_FIELD = 3

def main():

    if len(sys.argv[1:]) != 1:
        sys.stderr.write('Usage: {} moses.ini <nbest.with-new-features >nbest.rescored\n'.format(sys.argv[0]))
        sys.stderr.write('Entries are _not_ re-sorted based on new score.  Use topbest.py\n')
        sys.exit(2)

    weights = {}

    # moses.ini
    ini = open(sys.argv[1])
    while True:
        line = ini.readline()
        if not line:
            sys.stderr.write('Error: no [weight] section\n')
            sys.exit(1)
        if line.strip() == '[weight]':
            break
    while True:
        line = ini.readline()
        if not line or line.strip().startswith('['):
            break
        if line.strip() == '':
            continue
        fields = line.split()
        weights[fields[0]] = [float(f) for f in fields[1:]]

    # N-best
    for line in sys.stdin:
        fields = [f.strip() for f in line.split('|||')]
        feats = fields[FEAT_FIELD].split()
        key = ''
        i = 0
        score = 0
        for f in feats:
            if f.endswith('='):
                key = f
                i = 0
            else:
                score += (float(f) * weights[key][i])
                i += 1
        fields[SCORE_FIELD] = str(score)
        sys.stdout.write('{}\n'.format(' ||| '.join(fields)))

if __name__ == '__main__':
    main()
