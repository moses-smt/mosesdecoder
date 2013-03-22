#!/usr/bin/env python

#
# Implementation of PRO training and extensions to train phrase weights
#

import gzip
import logging
from numpy import array
import optparse
import os.path
import sys

from nbest import *
from sampler import *
from train import *


logging.basicConfig(format = "%(asctime)-15s %(message)s")
log = logging.getLogger('main')
log.setLevel(logging.DEBUG)
  
class Config:
  def __init__(self):
    self.parser = optparse.OptionParser(usage="%prog [options] ")
    self.parser.add_option("-t", "--trainer", action="store",\
      dest="trainer", metavar="TYPE", type="choice", choices=("pro","mix"),\
      default="pro",\
      help="type of trainer to run (pro,mix)")
    self.parser.add_option("-n", "--nbest", action="append", \
      dest="nbest", metavar="NBEST-FILE",\
      help="nbest output file(s) from decoder")
    self.parser.add_option("-S", "--scfile", action="append",\
      dest="score", metavar="SCORE-FILE",\
      help="score file(s) from extractor (in same order as nbests)")
    self.parser.add_option("-p", "--phrase-table" , action="append",\
      dest="ttable", metavar="TTABLE",\
      help="ttable to be used in mixture model training")
    self.parser.add_option("-i", "--input-file", action="store",\
      dest="input_file", metavar="INPUT-FILE",
      help="source text file")
    self.parser.add_option("-m", "--moses-bin-dir", action="store",\
      dest="moses_bin_dir", metavar="DIR",
      help="directory containing Moses binaries",
      default=os.path.expanduser("~/moses/bin"))
    self.nbest_files = []
    self.score_files = []
    self.ttables = []

  def parse(self,args=sys.argv[1:]):
    (options,args) = self.parser.parse_args(args)
    self.nbest_files = options.nbest
    self.score_files = options.score
    self.ttables = options.ttable
    self.input_file = options.input_file
    self.trainer = options.trainer
    self.moses_bin_dir = options.moses_bin_dir
    if not self.nbest_files:
      self.nbest_files = ["data/esen.nc.nbest.segment"]
    if not self.score_files:
      self.score_files = ["data/esen.nc.scores"]
    if len(self.nbest_files) != len(self.score_files):
      self.parser.error("Must have equal numbers of score files and nbest files")
    if self.trainer == "mix":
      if not self.input_file or not self.ttables:
        self.parser.error("Need to specify input file and ttables for mix training")
      #if len(self.ttables) != 2:
      #  self.parser.error("Can only train mix model with 2 ttables at the moment")

def main():
  config = Config()
  config.parse()

  samples = []
  sampler = HopkinsMaySampler()
  nbests = 0
  for nbest_file,score_data_file in zip(config.nbest_files,config.score_files):
    log.debug("nbest: " + nbest_file + "; score:" + score_data_file)
    segments = False
    if config.trainer == "mix": segments = True
    for nbest in get_scored_nbests(nbest_file, score_data_file, config.input_file, segments=segments):
        samples += sampler.sample(nbest)
        nbests += 1
    log.debug("Samples loaded")
  trainer = None
  if config.trainer == "mix":
    # Add the phrase table scores
    scorer = MosesPhraseScorer(config.ttables)
    log.debug("Scoring samples...")
    for sample in samples:
      scorer.add_scores(sample.hyp1)
      scorer.add_scores(sample.hyp2)
    log.debug("...samples scored")
    trainer = MixtureModelTrainer(samples)
  elif config.trainer == "pro":
    trainer = ProTrainer(samples)
  else: assert(0)
  log.debug("Starting training...")
  weights,mix_weights = trainer.train(debug=False)
  log.debug("...training complete")
  for i,w in enumerate(weights):
    print "F%d %10.8f" % (i,w)
  for i,f in enumerate(mix_weights):
    for j,w in enumerate(f):
      print "M%d_%d %10.8f" % (i,j,w)





if __name__ == "__main__":
  main()
