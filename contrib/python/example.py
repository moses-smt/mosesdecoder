from moses.dictree import load
import sys

if len(sys.argv) != 4:
    print "Usage: %s table nscores tlimit < query > result" % (sys.argv[0])
    sys.exit(0)

path = sys.argv[1]
nscores = int(sys.argv[2])
tlimit = int(sys.argv[3])

table = load(path, nscores, tlimit)

for line in sys.stdin:
    f = line.strip()
    result = table.query(f)
    # you could simply print the matches
    # print '\n'.join([' ||| '.join((f, str(e))) for e in matches])
    # or you can use its attributes
    print result.source
    for e in result:
        if e.lhs:
            print '\t%s -> %s ||| %s ||| %s' % (e.lhs, 
                    ' '.join(e.rhs), 
                    e.scores, 
                    e.alignment)
        else:
            print '\t%s ||| %s ||| %s' % (' '.join(e.rhs), 
                    e.scores, 
                    e.alignment)
     

