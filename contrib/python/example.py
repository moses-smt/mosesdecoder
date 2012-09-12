from binpt import BinaryPhraseTable
from binpt import QueryResult

import sys

if len(sys.argv) < 3:
    print "Usage: %s phrase-table nscores [wa] < query > result" % (sys.argv[0])
    sys.exit(0)

pt_file = sys.argv[1]
nscores = int(sys.argv[2])
wa = len(sys.argv) == 4

print >> sys.stderr, "-ttable %s -nscores %d -alignment-info %s\n" %(pt_file, nscores, str(wa))

pt = BinaryPhraseTable(pt_file, nscores, wa)
for line in sys.stdin:
    f = line.strip()
    matches = pt.query(f)
    print '\n'.join([' ||| '.join((f, str(e))) for e in matches])
    '''
    # This is how one would use the QueryResult object
    for e in matches:
        print ' '.join(e.words) # tuple of strings
        print e.scores # tuple of floats
        if e.wa:
            print e.wa # string
    '''
        
 

