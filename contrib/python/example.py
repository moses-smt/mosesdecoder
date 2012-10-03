import binpt
#from binpt import QueryResult
import sys


if len(sys.argv) < 3:
    print "Usage: %s phrase-table nscores [wa] < query > result" % (sys.argv[0])
    sys.exit(0)

pt_file = sys.argv[1]
nscores = int(sys.argv[2])
wa = len(sys.argv) == 4

pt = binpt.BinaryPhraseTable(pt_file, nscores, wa)
print >> sys.stderr, "-ttable %s -nscores %d -alignment-info %s -delimiter '%s'\n" %(pt.path, pt.nscores, str(pt.wa), pt.delimiters)

for line in sys.stdin:
    f = line.strip()
    matches = pt.query(f, cmp = binpt.QueryResult.desc, top = 20)
    print '\n'.join([' ||| '.join((f, str(e))) for e in matches])
    '''
    # This is how one would use the QueryResult object
    for e in matches:
        print ' '.join(e.words) # tuple of strings
        print e.scores # tuple of floats
        if e.wa:
            print e.wa # string
    '''
            
     

