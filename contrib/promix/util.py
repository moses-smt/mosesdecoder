#!/usr/bin/python

import numpy as np


floor = np.exp(-100)
def safelog(x):
  """Wraps np.log to give it a floor"""
  #return np.log(x)
  return np.log(np.clip(x,floor,np.inf))


