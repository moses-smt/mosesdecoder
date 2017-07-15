#!/usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

import sys

SCORE_FIELD = 3

def main():

    i = ''
    hyp = ''
    top = 0

    for line in sys.stdin:
        fields = [f.strip() for f in line.split('|||')]
        id = fields[0]
        if i != id:
            if i:
                sys.stdout.write('{}\n'.format(hyp))
        score = float(fields[SCORE_FIELD])
        if score > top or i != id:
            i = id
            hyp = fields[1]
            top = score
    sys.stdout.write('{}\n'.format(hyp))

if __name__ == '__main__':
    main()
