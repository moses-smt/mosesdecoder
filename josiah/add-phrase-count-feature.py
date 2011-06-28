#!/usr/bin/env python


import gzip
import optparse
import sys

#
# Add features to phrase table based on bins
#

def add_bin_features(bins,count,fields):
  bin = 0
  for i,max in enumerate(bins):
    if count >= max: bin = i+1
  features = [1] * (len(bins)+1)
  features[bin] = 2.718
  features = [str(f) for f in features]
  fields[2] = fields[2] + " ".join(features) + " "


def main():
  parser = optparse.OptionParser(
    usage = "usage: %prog [options] in-phrase-table out-phrase-table")

  parser.add_option("-b", "--bin", action="append", dest="bins",
    metavar="BIN", type="int", help="Bin boundary (upper, exclusive)")
  parser.add_option("-s", "--source", action="store_true", dest="source",
    help="Add features based on source counts")
  parser.add_option("-t", "--target", action="store_true", dest="target",
    help="Add features based on target counts")
  parser.set_default("source",False)
  parser.set_default("target",False)

  (options,args) = parser.parse_args()
  if len(args) != 2:
    parser.error("Need to specify input and output phrase tables")

  use_source = options.source
  use_target = options.target
  bins = options.bins
  bins.sort()

  tt_in = gzip.GzipFile(args[0])
  tt_out = gzip.GzipFile(args[1],"w")
  for line in tt_in:
    fields = line[:-1].split("||| ")
    counts = [int(f) for f in fields[4].split()]
    if use_source:
      add_bin_features(bins,counts[0],fields)
    if use_target:
      add_bin_features(bins,counts[1],fields)
    print>>tt_out,"||| ".join(fields) 


if __name__ == "__main__":
  main()
