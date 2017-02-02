#!/usr/bin/env python
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

import argparse
import os
import os.path
import sys


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("-w", "--working-dir",  dest="working_dir")
  parser.add_argument("-n", "--order",  dest="n")
  parser.add_argument("-i", "--ini_filename",  dest="ini_filename")
  parser.add_argument("-x", "--name",  dest="name")
  parser.add_argument("-e", "--epochs",  dest="epochs")
  parser.add_argument("-f", "--factor",  dest="factor")
  

  parser.set_defaults(
    working_dir="working",
    n = "5",
    ini_filename = "nplm.ini",
    name = "neural",
    epochs = "10",
    factor = "0"
  )

  options = parser.parse_args()

  if not os.path.exists(options.working_dir):
      os.makedirs(options.working_dir)


  ini_filename = os.path.join(options.working_dir,options.ini_filename)
  path = "%s/train.model.nplm.%s" % (options.working_dir, "best")
  if not os.path.exists(path):
    path = "%s/train.model.nplm.%s" % (options.working_dir, options.epochs)


  with open(ini_filename,"w") as ifh:
    print>>ifh, "[feature]"
    print>>ifh,"NeuralLM factor=%s name=NPLM%s order=%s path=%s" \
      % (options.factor,options.name, options.n, path)
    print>>ifh
    print>>ifh,"[weight]"
    print>>ifh,"NPLM%s= 0.1" % options.name
    print>>ifh


if __name__ == "__main__":
  main()

