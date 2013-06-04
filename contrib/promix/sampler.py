#!/usr/bin/env python

#
#
#

import heapq
import math
import random
import sys

from bleu import BleuScorer


class Sample:
  """A pair of hypotheses, and their score difference"""
  def __init__(self,hyp1,hyp2):
    self.hyp1 = hyp1
    self.hyp2 = hyp2
    self.diff = abs(hyp1.score-hyp2.score)

  def __cmp__(self,other):
    return cmp(self.diff,other.diff)

class HopkinsMaySampler:
  """Implements Hopkins & May sampling"""
  def __init__(self):
    self.ncandidates = 5000 # Gamma in Hopkins and May
    self.nsamples = 50 # Xi in Hopkins and May
    self.min_diff = 0.05 # Minimum scoring difference

  def sample(self,nbest):
    samples = []
    for i in xrange(self.ncandidates):
      hyp1 = random.choice(nbest.hyps)
      hyp2 = random.choice(nbest.hyps)
      sample = Sample(hyp1,hyp2)
      if  sample.diff < self.min_diff: continue
      # maintain nsamples biggest samples
      heapq.heappush(samples,sample)
      while len(samples) > self.nsamples:
        heapq.heappop(samples)
    return samples

