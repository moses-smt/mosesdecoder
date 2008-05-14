#/usr/bin/python

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

    # pick sentences to score with
    index = 0
    cookedtests = []
    for i  in range(len(tests)):
        sentence = tests[i][index]
        cookedtest = (bleu.cook_test(sentence, cookedrefs[i]))
        stats = " ".join(["%d %d" % (c,g) for (c,g) in zip(cookedtest['correct'], cookedtest['guess'])])
        print " %s %d" % (stats ,cookedtest['reflen'])
        cookedtests.append(cookedtest)
        index = index + 1
        if index == 10:
            index = 0

    bleu = bleu.score_cooked(cookedtests)
    print "Bleu: ", bleu
    
if __name__ == "__main__":
    main()
