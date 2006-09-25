#!/usr/bin/python2.3

"""Convert n-best list in mert.perl format to format required by
Venugopal's MER trainer. This entails calculating the BLEU component scores."""

"""usage: score-nbest.py <reffile>+ <outprefix>

   The input should be sorted by sentence number and piped into stdin
   Run it like this: sort -mnk 1,1 *.nbest | score-nbest.py ...
"""

import sys, itertools, re
import bleu
#The default python version on DICE is currently 2.3, which does not contain sets as a built-in module.
#Comment out this line when moving to python 2.4
from sets import Set as set

def process(sentnum, testsents):
    candsfile.write("%d %d\n" % (cur_sentnum, len(testsents)))
    for (sent,vector) in testsents:
        comps = bleu.cook_test(sent, cookedrefs[sentnum])

        if comps['testlen'] != comps['guess'][0]:
            sys.stderr.write("ERROR: test length != guessed 1-grams\n")
	featsfile.write("%s %s %d\n" % (" ".join([str(v) for v in vector]),
					    " ".join(["%d %d" % (c,g) for (c,g) in zip(comps['correct'], comps['guess'])]),
					    comps['reflen']))

if __name__ == "__main__":
    import psyco
    psyco.full()

    import getopt
    (opts,args) = getopt.getopt(sys.argv[1:], "casen", [])

    for (opt,parm) in opts:
        if opt == "-c":
            bleu.preserve_case = True
        if opt == "-a":
            bleu.eff_ref_len = "average"
        if opt == "-s":
            bleu.eff_ref_len = "shortest"
        if opt == "-e":
            bleu.eff_ref_len = "closest"
        if opt == "-n":
            bleu.nonorm = 1

    print args    
    cookedrefs = []
    reffiles = [file(name) for name in args[:-1]]
    print reffiles
    for refs in itertools.izip(*reffiles):
        cookedrefs.append(bleu.cook_refs(refs))
    
    outprefix = args[-1]

    featsfile = file(outprefix+"feats.opt", "w")
    candsfile = file(outprefix+"cands.opt", "w")

    cur_sentnum = None
    testsents = set()
    progress = 0

    infile = sys.stdin

    # function that recognizes floats
    re_float=re.compile(r'^-?[-0-9.e]+$')
    is_float=lambda(x):re_float.match(x)

    for line in infile:
        try:
            ##Changed to add a further field - AA 29/11/05
            #(sentnum, sent, vector) = line.split('|||')
            (sentnum, sent, vector, prob ) = line.split('|||')
        except:
            sys.stderr.write("ERROR: bad input line %s\n" % line)
        sentnum = int(sentnum)
        sent = " ".join(sent.split())
	# filter out score labels (keep only floats) and convert numbers to floats
        vector = tuple(map(lambda(s): -float(s), filter(is_float, vector.split())))

        if sentnum != cur_sentnum:
            if cur_sentnum is not None:
                process(cur_sentnum, testsents)
            cur_sentnum = sentnum
            testsents = set()
        testsents.add((sent,vector))

        if progress % 10000 == 0:
            sys.stdout.write(".")
            sys.stdout.flush()

        progress += 1
    process(cur_sentnum, testsents)

    sys.stdout.write("\n")
    featsfile.close()
    candsfile.close()
        
            
    

