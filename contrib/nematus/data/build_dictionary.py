#!/usr/bin/python

import numpy
import json

import sys
import fileinput

from collections import OrderedDict

def main():
    for filename in sys.argv[1:]:
        print 'Processing', filename
        word_freqs = OrderedDict()
        with open(filename, 'r') as f:
            for line in f:
                words_in = line.strip().split(' ')
                for w in words_in:
                    if w not in word_freqs:
                        word_freqs[w] = 0
                    word_freqs[w] += 1
        words = word_freqs.keys()
        freqs = word_freqs.values()

        sorted_idx = numpy.argsort(freqs)
        sorted_words = [words[ii] for ii in sorted_idx[::-1]]

        worddict = OrderedDict()
        worddict['eos'] = 0
        worddict['UNK'] = 1
        for ii, ww in enumerate(sorted_words):
            worddict[ww] = ii+2

        with open('%s.json'%filename, 'wb') as f:
            json.dump(worddict, f, indent=2, ensure_ascii=False)

        print 'Done'

if __name__ == '__main__':
    main()
