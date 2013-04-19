#!/usr/bin/env python

import random
import unittest

from nbest import *
from sampler import *


class TestNBestSampler(unittest.TestCase):

  def setUp(self):
    self.h1 = Hypothesis("a",[])
    self.h2 = Hypothesis("b",[])
    self.h3 = Hypothesis("c",[])
    self.nbest = NBestList(1)
    self.nbest.hyps.append(self.h1)
    self.nbest.hyps.append(self.h2)
    self.nbest.hyps.append(self.h3)
    self.sampler = HopkinsMaySampler()
    
  def test_nsamples(self):
    self.h1.score = 0.1
    self.h2.score = 0.2
    self.h3.score = 0.3
    samples = self.sampler.sample(self.nbest)
    self.assertEqual(len(samples), self.sampler.nsamples)
    
  def test_biggest(self):
    random.seed(0)
    self.h1.score = 0.1
    self.h2.score = 0.2
    self.h3.score = 0.3
    samples = self.sampler.sample(self.nbest)
    for sample in samples:
      self.assertAlmostEqual(sample.diff,0.2)
   
  def test_score_diff(self):
    self.h1.score = 0.1
    self.h2.score = 0.1 + (0.9*self.sampler.min_diff)
    self.h3.score = 0.1 + (1.8*self.sampler.min_diff)

    # Should only see pairs with h1,h3
    samples = self.sampler.sample(self.nbest)
    for sample in samples:
      self.assertTrue((sample.hyp1 == self.h1 and sample.hyp2 == self.h3) or \
                 (sample.hyp2 == self.h1 and sample.hyp1 == self.h3))


if __name__ == "__main__":
  unittest.main()

suite = unittest.TestLoader().loadTestsFromTestCase(TestNBestSampler)
