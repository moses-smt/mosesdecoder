#!/usr/bin/env python

import math
import unittest

from bleu import *

class TestBleuScorer(unittest.TestCase):
  def setUp(self):
    self.scorer = BleuScorer()

  def test_wrong_length(self):
    self.assertRaises(BleuScoreException, self.scorer.score, [1]*8)
    self.assertRaises(BleuScoreException, self.scorer.score, [1]*10)

  def test_score(self):
    stats = [2,5,0,1,1,1,1,1,5]
    self.assertAlmostEqual(self.scorer.score(stats), 1/math.sqrt(2))

  def test_brevity(self):
    stats = [2,2,2,2,2,2,2,2,3]
    self.assertAlmostEqual(self.scorer.score(stats), math.exp(1 - 3.0/2.0))

if __name__ == "__main__":
  unittest.main()

suite = unittest.TestLoader().loadTestsFromTestCase(TestBleuScorer)

