#!/usr/bin/env python

import math
import unittest

from moses import *

class TestMoses(unittest.TestCase):

  def setUp(self):
    self.ttable = PhraseTable("data/esen.ep.model.unfiltered/phrase-table",5,True)
  
  def test_hw(self):
    self.assertEqual(greet(), "hello, world")

  def test_get_translations(self):
    translations = self.ttable.get_translations("lados")
    self.assertEqual(len(translations),61)
    self.assertEqual(translations[0][1], [0.00015135499415919185, 0.00063730002148076892, 0.0018050499493256211, 0.011290299706161022, 2.7179999351501465])

  def test_get_translation_probability(self):
    fv = self.ttable.get_translation_probability("en ambos", "on both")
    expected_fv = [0.0908379,0.0213197,0.187399,0.0498198,2.718]
    for i in range(5):
      self.assertAlmostEqual(fv[i], expected_fv[i], 4)
    fv = self.ttable.get_translation_probability("kljhdklj!", "hkhdkj")
    for i in range(5):
      self.assertEqual(fv[i], 0)
    

if __name__ == "__main__":
  unittest.main()

suite = unittest.TestLoader().loadTestsFromTestCase(TestMoses)

