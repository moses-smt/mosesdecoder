#!/usr/bin/env python
# compute Bleu scores with confidence intervals via boostrap resampling
# written by Ulrich Germann
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

from argparse import ArgumentParser
import math
import os
from random import randint
import sys, gzip


def count_ngrams(snt, max_n):
    """
    Return a dictionary of ngram counts (up to length /max_n/)
    for sentence (list of words) /snt/.
    """
    ret = {}
    for i in xrange(len(snt)):
        for k in xrange(i + 1, min(i + max_n + 1, len(snt) + 1)):
            key = tuple(snt[i:k])
            ret[key] = ret.get(key, 0) + 1
    return ret


def max_counts(ng1, ng2):
    """
    Return a dicitonary of ngram counts such that
    each count is the greater of the two individual counts
    for each ngram in the input ngram count dictionaries
    /ng1/ and /ng2/.
    """
    ret = ng1.copy()
    for k, v in ng2.items():
        ret[k] = max(ret.get(k, 0), v)
    return ret


def ng_hits(hyp, ref, max_n):
    """
    Return a list of ngram counts such that each ngram count
    is the minimum of the counts in hyp and ref, up to ngram
    length /max_n/.
    """
    ret = [0 for i in xrange(max_n)]
    for ng, cnt in hyp.items():
        k = ng
        if len(k) <= max_n:
            ret[len(k) - 1] += min(cnt, ref.get(ng, 0))
    return ret


class BleuScore:
    def __init__(self, hyp, ref, max_n=4, bootstrap=1000):
        # print len(hyp.ngrams), len(ref.ngrams), "X"
        self.hits = [
            ng_hits(hyp.ngrams[i], ref.ngrams[i], max_n)
            for i in xrange(len(hyp.ngrams))]
        self.max_n = max_n
        self.hyp = hyp
        self.ref = ref
        self.lower = None
        self.upper = None
        self.median = None
        self.actual = self.score([i for i in xrange(len(hyp.snt))])
        if bootstrap:
            self.bootstrap = [self.score([randint(0, len(hyp.snt) - 1)
                                          for s in hyp.snt])
                              for i in xrange(bootstrap)]
            self.bootstrap.sort()
        else:
            self.bootstrap = [self.actual]
            pass

    def score(self, sample):
        hits = [0 for i in xrange(self.max_n)]
        self.hyplen = 0
        self.reflen = 0
        self.total = [0 for i in hits]
        for i in sample:
            self.hyplen += len(self.hyp.snt[i])
            self.reflen += len(self.ref.snt[i])
            for n in xrange(self.max_n):
                hits[n] += self.hits[i][n]
                self.total[n] += max(len(self.hyp.snt[i]) - n, 0)
                pass
        self.prec = [float(hits[n]) / self.total[n]
                     for n in xrange(self.max_n)]
        ret = sum([math.log(x) for x in self.prec]) / self.max_n
        self.BP = min(
            1, math.exp(1. - float(self.reflen) / float(self.hyplen)))
        ret += math.log(self.BP)
        return math.exp(ret)


class Document:
    def __init__(self, fname=None):
        self.fname = fname
        if fname:
            if fname[-3:] == ".gz":
                self.snt = [line.strip().split() for line in gzip.open(fname).readlines()]
            else:
                self.snt = [line.strip().split() for line in open(fname)]
                pass
            self.ngrams = [count_ngrams(snt, 4) for snt in self.snt]
            # print self.snt
        else:
            self.snt = None
            self.ngrams = None

    def merge(self, R):
        self.fname = "multi-ref"
        self.ngrams = [x for x in R[0].ngrams]
        self.snt = [x for x in R[0].snt]
        for i in xrange(len(R[0].ngrams)):
            for k in xrange(1, len(R)):
                self.ngrams[i] = max_counts(self.ngrams[i], R[k].ngrams[i])

    def update(self, hyp, R):
        for i, hyp_snt in enumerate(hyp.snt):
            clen = len(hyp_snt)
            K = 0
            for k in xrange(1, len(R)):
                k_snt = R[k].snt[i]
                assert len(R[k].snt) == len(hyp.snt), (
                    "Mismatch in number of sentences " +
                    "between reference and candidate")
                if abs(len(k_snt) - clen) == abs(len(R[K].snt[i]) - clen):
                    if len(k_snt) < len(R[K].snt[i]):
                        K = k
                elif abs(len(k_snt) - clen) < abs(len(R[K].snt[i]) - clen):
                    K = k
            self.snt[i] = R[K].snt[i]


if __name__ == "__main__":
    argparser = ArgumentParser()
    argparser.add_argument(
        "-r", "--ref", nargs='+', help="Reference translation(s).")
    argparser.add_argument(
        "-c", "--cand", nargs='+', help="Candidate translations.")
    argparser.add_argument(
        "-a", "--all", action="store_true", help="show all ngrams")
    args = argparser.parse_args(sys.argv[1:])
    R = [Document(fname) for fname in args.ref]
    C = [Document(fname) for fname in args.cand]
    Rx = Document()  # for multi-reference BLEU
    Rx.merge(R)

    X = {}
    for sid in xrange(len(Rx.ngrams)):
        ref = Rx.ngrams[sid]
        for i in xrange(len(C)):
            for k,v in C[i].ngrams[sid].items():
                h = min(ref.get(k,0),v)
                if h == 0: continue
                x = X.setdefault(k, [0 for c in C])
                x[i] += h
                pass
            pass
        pass

    for k,v in X.items():
        if args.all:
            print v, " ".join(k)
        else:
            for i in xrange(1,len(v)):
                if v[i] != v[i-1]:
                    print abs(v[i]-v[i-1]), v, " ".join(k)
                    break
                pass
            pass
        pass
    
