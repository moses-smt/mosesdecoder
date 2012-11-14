from moses.dictree import PhraseDictionaryTree as BinaryPhraseTable
from moses.dictree import OnDiskWrapper as BinaryRuleTable
from moses.dictree import QueryResult
from moses.dictree import load
import sys

if len(sys.argv) < 2:
    print "Usage: %s table nscores < query > result" % (sys.argv[0])
    sys.exit(0)

path = sys.argv[1]
nscores = int(sys.argv[2])

table = load(path, nscores)

for line in sys.stdin:
    f = line.strip()
    matches = table.query(f, cmp = QueryResult.desc, top = 20)
    print '\n'.join([' ||| '.join((f, str(e))) for e in matches])
     

