#!/usr/bin/env python

from  math import exp,log

class BleuScoreException(Exception):
  pass

class BleuScorer:
  def __init__(self):
    """References should be a list. If each element is a string, assume
    they are filenames, if a list, assume tokenised strings"""
    self.smooth = 1.0
    self.order = 4

  def score(self,scores):
    if len(scores) != self.order*2+1:
      raise BleuScoreException("Wrong number of scores. Expected %d, but found %d" %
        (self.order*2+1, len(scores)))
    logbleu = 0.0
    for j in range(self.order):
      logbleu += log(scores[2*j] + self.smooth) - log(scores[2*j+1] + self.smooth)
    logbleu /= self.order
    brevity = 1.0 - float(scores[-1]) / scores[1]
    if brevity < 0:
      logbleu += brevity
    return exp(logbleu)
    
