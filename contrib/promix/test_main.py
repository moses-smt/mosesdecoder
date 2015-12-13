#!/usr/bin/env python

import unittest

from main import *

class TestConfig(unittest.TestCase):
  def setUp(self):
    self.config = Config()

  def test_files_short(self):
    nbests = ["nbest1", "nbest2", "nbest3"]
    scores = ["score1", "score2", "score3"]
    self.config.parse(["-n", nbests[0], "-n", nbests[1], "-n", nbests[2],\
     "-S", scores[0],  "-S", scores[1], "-S", scores[2]])
    self.assertEqual(self.config.nbest_files, nbests)
    self.assertEqual(self.config.score_files, scores)


if __name__ == "__main__":
    unittest.main()


suite = unittest.TestLoader().loadTestsFromTestCase(TestConfig)
