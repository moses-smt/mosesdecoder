#!/usr/bin/env python
# coding=utf8

import numpy as np
import numpy.testing as nptest
import os.path
import unittest

from nbest import *

class TestReadNBest(unittest.TestCase):
  def setUp(self):
    self.nbest = "test_data/test.nbest.nbest"
    self.nbest_segment = "test_data/test.nbest.nbest.segments"
    self.scores = "test_data/test.nbest.scores"
    self.input = "test_data/test.nbest.input"

  def test_featureindex(self):
    for nbest in get_scored_nbests(self.nbest,self.scores,self.input):
      pass
    self.assertEqual(get_feature_index("tm"), [9,14])
    self.assertEqual(get_feature_index("lm"), [7,8])
    self.assertEqual(get_feature_index("d"), [0,7])
    self.assertEqual(get_feature_index("w"), [8,9])

  def test_nosegment(self):
    count = 0
    for nbest in get_scored_nbests(self.nbest,self.scores,self.input):
      count += 1
      hyp0 = nbest.hyps[0]
      expected_fv = np.array([0,-1.51037,0,0,-2.60639,0,0 ,-36.0562, -8,-5.97082,-14.8327,-2.41162,-9.32734,3.99959])
      self.assertEqual(len(hyp0.fv), len(expected_fv))
      for i in range(len(hyp0.fv)):
        self.assertAlmostEqual(expected_fv[i],hyp0.fv[i])
      self.assertEqual(hyp0.text,"taming politicians on both sides of the Atlantic")
    self.assertEqual(count,1)

  def test_segment(self):
    count = 0
    for nbest in get_scored_nbests(self.nbest_segment,self.scores,self.input, segments=True):
      count += 1
      hyp0 = nbest.hyps[0]
      self.assertEqual(hyp0.text,"taming politicians on both sides of the Atlantic")
      expected_align = [(0,2,1), (2,4,2), (4,6,4), (6,9,8)]
      self.assertEqual(hyp0.alignment, expected_align)
    self.assertEqual(count,1)

class TestMosesPhraseScorer(unittest.TestCase):

  def setUp(self):
    self.scorer = MosesPhraseScorer\
      (("test_data/esen.nc.model.filtered/phrase-table.0-0.1.1", \
                      "test_data/esen.ep.model.filtered/phrase-table.0-0.1.1"))

  def test_phrase_scores(self):
    hyp0 = Hypothesis("taming |0-1| politicians |2-3| on both |4-5| sides of the Atlantic |6-8|", [0, -1.51037,0, 0, -2.60639, 0, 0, -36.0562,-8,-5.97082,-14.8327,-2.41162,\
      -9.32734,3.99959], True)
    hyp0.input_line = "domando a los políticos en ambos lados del Atlántico"
    #hyp0.score = 0.2140
    self.scorer.add_scores(hyp0)
    self.assertEqual(len(hyp0.phrase_scores),2)
    # Each ttable should provide 4 sets of 4 scores (ignore penalty)
    # These are the probabilities
    # nc first, then ep. Columns are different features
    expected = np.array([\
      [[0.0740741,0.00756144,1,0.500047],\
       [0.545866,0.233725,0.818336,0.186463],\
       [0.288344,0.0548148,0.335714,0.0543585],\
       [0.777778,0.0122444,0.777778,0.0351361]],\
      [[0,0,0,0],\
       [0.33497, 0.180441, 0.638586, 0.0962213],\
       [0.0908379,0.0213197,0.187399,0.0498198],
       [0.62585,0.00702384,0.836364,0.0687874]]\
      ])

    nptest.assert_almost_equal(hyp0.phrase_scores, expected)

    # These are the interpolation weights reported by tmcombine
    weights = np.array([[0.54471993730312251, 0.45528006269687754],\
               [0.56546688367708142, 0.43453311632291863],\
               [0.55867730373453584, 0.44132269626546422],\
               [0.46645964485220004, 0.53354035514779996]])

    #check that the scores are interpolated as expected 
    interpolated_probs = expected[0]*weights[:,0] + expected[1]*weights[:,1]
    interpolated_scores = np.log(interpolated_probs)
    # each column corresponds to a feature
    expected_fv = interpolated_scores.sum(axis=0)
    for i in range(4):
      self.assertAlmostEqual(hyp0.fv[9+i], expected_fv[i], places=4)

class TestPhraseCache (unittest.TestCase):
  def test_add_get(self):
    """Add something to cache and check we can get it back"""
    cache = PhraseCache(10)
    self.assertFalse(cache.get("aa", "bb"))
    scores = [1,2,3,4,5]
    cache.add("aa", "bb", scores)
    self.assertEquals(cache.get("aa","bb"),scores)
    self.assertFalse(cache.get("aa","cc")) 
    self.assertFalse(cache.get("cc","bb")) 

  def test_lru(self):
    """Check that items are cleared from the cache"""
    cache = PhraseCache(2)
    s1 = [1,2,3,4,5]
    s2 = [2,3,4,5,6]
    s3 = [3,4,5,6,7]
    cache.add("aa","bb",s1)
    cache.add("bb","cc",s2)
    cache.add("dd","ee",s3)
    self.assertEquals(cache.get("dd","ee"), s3)
    self.assertEquals(cache.get("bb","cc"), s2)
    self.assertFalse(cache.get("aa","bb"))
    cache.add("aa","bb",s1)
    self.assertFalse(cache.get("dd","ee"))




if __name__ == "__main__":
  unittest.main()

suite = unittest.TestSuite([unittest.TestLoader().loadTestsFromTestCase(TestReadNBest), unittest.TestLoader().loadTestsFromTestCase(TestMosesPhraseScorer)])
