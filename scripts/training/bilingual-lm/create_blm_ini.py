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
  parser.add_argument("-n", "--target-context",  dest="n")
  parser.add_argument("-m", "--source-context",  dest="m")
  parser.add_argument("-i", "--ini_filename",  dest="ini_filename")
  parser.add_argument("-x", "--name",  dest="name")
  parser.add_argument("-e", "--epochs",  dest="epochs")
  

  parser.set_defaults(
    working_dir="working",
    n = "5",
    m = "4",
    ini_filename = "blm.ini",
    name = "comb",
    epochs = "10"
  )

  options = parser.parse_args()

  if not os.path.exists(options.working_dir):
      os.makedirs(options.working_dir)

  # Bit of a hack, parse the working directory to get the name
  name = os.path.basename(options.working_dir).split(".")[0].split("-")[-1]

  ini_filename = os.path.join(options.working_dir,options.ini_filename)
  with open(ini_filename,"w") as ifh:
    print>>ifh, "[feature]"
    print>>ifh,"BilingualNPLM name=BLM%s order=%s source_window=%s path=%s/train.10k.model.nplm.%s source_vocab=%s/vocab.source target_vocab=%s/vocab.target" \
      % (options.name,options.n, options.m, options.working_dir, options.epochs, options.working_dir, options.working_dir)
    print>>ifh
    print>>ifh,"[weight]"
    print>>ifh,"BLM%s= 0.1" % options.name
    print>>ifh


if __name__ == "__main__":
  main()

