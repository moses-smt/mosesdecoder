#!/usr/bin/python

#
# Calculate bleu score for test files using old (python) script
#

import os.path
import sys


def main():
    sys.path.append("../scripts/training/cmert-0.5")
    import bleu
    data_dir = "test_scorer_data"
    nbest_file = os.path.join(data_dir,"nbest.out")
    ref_file = os.path.join(data_dir,"reference.txt")
    bleu.preserve_case = False
    bleu.eff_ref_len = "shortest"
    bleu.nonorm = 0

    ref_fh = open(ref_file)
    cookedrefs = []
    for ref in ref_fh:
        cookedref = bleu.cook_refs([ref])
        cookedrefs.append(cookedref)
    ref_fh.close()
    
    nbest_fh = open(nbest_file)
    tests = []
    i = -1
    for line in nbest_fh:
        fields = line.split("||| ")
        current_i = int(fields[0])
        text = fields[1]
        if i != current_i:
            tests.append([])
            i = current_i
        tests[-1].append(text)
    nbest_fh.close()

    #  score with first best
    cookedtests = []
    for i  in range(len(tests)):
        sentence = tests[i][0]
        cookedtest = (bleu.cook_test(sentence, cookedrefs[i]))
        stats = " ".join(["%d %d" % (c,g) for (c,g) in zip(cookedtest['correct'], cookedtest['guess'])])
        print " %s %d" % (stats ,cookedtest['reflen'])
        cookedtests.append(cookedtest)
    bleu1 = bleu.score_cooked(cookedtests)

    # vary, and score again
    cookedtests = []
    for i in range(len(tests)):
        sentence = tests[i][0]
        if i == 7:
            sentence = tests[i][8]
        elif i == 1:
            sentences = tests[i][2]
        cookedtest = (bleu.cook_test(sentence, cookedrefs[i]))
        cookedtests.append(cookedtest)
    bleu2 = bleu.score_cooked(cookedtests)
    

    print "Bleus: ", bleu1,bleu2
    
if __name__ == "__main__":
    main()
