#!/usr/bin/env python

#
# Filter a parallel corpus
#

import heapq
import logging
import math
import optparse
import random
import sys

import numpy

from defaultconfig import Config

logging.basicConfig(format = "%(asctime)-15s %(message)s")
log = logging.getLogger("filter")
log.setLevel(logging.DEBUG)

class FilterStrategy(object):
  def __init__(self,config):
    pass

  def filter(self,source,target):
    return True


class RandomFilterStrategy(FilterStrategy):
  def __init__(self,config):
    self.threshold = config.getfloat("random", "threshold", 0.1)
    random.seed()

  def filter(self, source, target):
    return random.random() < self.threshold


class ScoreFilterStrategy(FilterStrategy):
  """Filter strategy that is based on a file with sentence scores. There are three
  possible ways of specifying how to filter:
    i) threshold - filter all sentence pairs whose score is less than the threshold
    ii) proportion - filter all but a certain proportion (eg a tenth) of the sentences
    iii) count - filter all but a given count of the sentences.
    """
  def __init__(self,config):
    section = "score"
    self.score_file = config.get(section,"score_file")
    option_names = ("threshold", "proportion", "count")
    options = [config.config.has_option(section,o) for o in option_names]
    if sum(options) != 1:
      raise RuntimeError("Must specify exactly one of %s for score filter" % str(option_names)) 
    if options[0]:
      # threshold
      self.threshold = config.getfloat(section,option_names[0])
    else:
      # proportion or count
      if options[2]:
        count = config.getint(section,option_names[2])
      else:
        # need to count entries
        count = 0
        for line in open(self.score_file): count = count + 1
        count = int(count * config.getfloat(section,option_names[1]))
      log.info("Retaining %d entries" % count)
      # Find the threshold
      self.threshold = sorted(\
        [float(line[:-1]) for line in open(self.score_file)], reverse=True)[count]
      #self.threshold = heapq.nlargest(count, \
      #  [float(line[:-1]) for line in open(self.score_file)])[-1]


    self.sfh = open(self.score_file) 
    log.info("Thresholding scores at " + str(self.threshold))

  def filter(self,source,target):
    score = self.sfh.readline()
    if not score:
      raise RuntimeError("score file truncated")
    return float(score[:-1]) >= self.threshold
  

def main():
  parser = optparse.OptionParser(usage = "Usage: %prog [options] config-file")
  (options,args) = parser.parse_args()
  if len(args) < 1:
    parser.error("No configuration file specified")

  log.info("Loading configuration from " + args[0])
  config = Config(args[0])
  log.debug("Configuration:\n" + str(config))

  # Required general parameters
  source_lang = config.get("general", "source_language")
  target_lang = config.get("general", "target_language")
  input_stem = config.get("general", "input_stem")
  output_stem = config.get("general", "output_stem")
  strategy = config.get("general", "strategy", "")

  # Optional general parameters
  alignment_stem = config.get("general", "alignment_stem", "")
  alignment_type = config.get("general", "alignment_type", "grow-diag-final-and")

  strategy_class = globals()[strategy + "FilterStrategy"]
  strategy = strategy_class(config)

  source_input_fh = open(input_stem + "." + source_lang)
  target_input_fh = open(input_stem + "." + target_lang)
  source_output_fh = open(output_stem + "." + source_lang, "w")
  target_output_fh = open(output_stem + "." + target_lang, "w")

  alignment_input_fh = None
  alignment_output_fh = None
  if alignment_stem:
    alignment_input_fh = open(alignment_stem + "." + alignment_type)
    alignment_output_fh = open(output_stem + "." + alignment_type,"w")

  retained = 0
  for source_line in source_input_fh:
    target_line = target_input_fh.readline()
    if alignment_input_fh:
      align_line = alignment_input_fh.readline()
    if strategy.filter(source_line,target_line):
      retained = retained + 1
      print>>source_output_fh, source_line,
      print>>target_output_fh, target_line,
      if alignment_input_fh:
        print>>alignment_output_fh, align_line,
  log.info("Lines retained: %d" % retained)

if __name__ == "__main__":
  main()
