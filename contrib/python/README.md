# Python interface to Moses

The idea is to have some of Moses' internals exposed to Python (inspired on pycdec).

## What's been interfaced?

* Binary tables:

        Moses::PhraseDictionaryTree
        OnDiskPt::OnDiskWrapper

## Building

1.  Build the python extension: 

    You need to compile Moses with link=shared

        ./bjam --libdir=path link=shared

    Then you can build the extension (in case you used --libdir=path above, use --moses-lib=path below) 

        python setup.py build_ext -i [--with-cmph] [--moses-lib=PATH] [--cython] [--max-factors=NUM] [--max-kenlm-order=NUM]

    Use `--cython` if you want to re-compile the pyx files, note that they already come compiled so that you don't need to have Cython installed 

## Example

### Getting a phrase table

    cd examples
    export LC_ALL=C
    cat phrase-table.txt | sort | ../../../bin/processPhraseTable -ttable 0 0 - -nscores 5 -alignment-info -out phrase-table

### Getting a rule table

    cd examples
    ../../../bin/CreateOnDiskPt 0 0 5 20 2 rule-table.txt rule-table

### Querying

1. Phrase-based
    
        echo "casa" | python example.py examples/phrase-table 5 20
        echo "essa casa" | python example.py examples/phrase-table 5 20

2. Hierarchical

        echo "i [X]" | python example.py examples/rule-table 5 20
        echo "have [X]" | python example.py examples/rule-table 5 20
        echo "[X][X] do not [X][X] [X]" | python example.py examples/rule-table 5 20

### Code

```python
from moses.dictree import load # load abstracts away the choice of implementation by checking the available files
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
    # or you can use their attributes
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
```


## Changing the code

If you want to add your changes you are going to have to recompile the cython code.

1.  Compile the cython code using Cython 0.17.1

    
        python setup.py build_ext -i --cython
 
