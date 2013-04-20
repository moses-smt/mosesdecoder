#!/usr/bin/env python

import unittest

import test_bleu
import test_main
import test_moses
import test_nbest
import test_sampler
import test_train

def main():
  test_list = []
  test_list.append(test_bleu.suite)
  test_list.append(test_main.suite)
#  test_list.append(test_moses.suite)
  test_list.append(test_nbest.suite)
  test_list.append(test_sampler.suite)
  test_list.append(test_train.suite)

  suite = unittest.TestSuite(test_list)
  unittest.TextTestRunner().run(suite)

if __name__ == "__main__":
  main()
