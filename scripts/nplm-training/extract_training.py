#!/usr/bin/env python

from collections import Counter
import logging
import optparse
import os
import os.path
import sys

import extract

LOG = logging.getLogger(__name__)

def get_pruned_vocab(corpus,prune):
  counts = Counter()
  LOG.info("Reading vocabulary from %s" % corpus)
  lines = 0
  for line in open(corpus):
    for token in line[:-1].split():
      counts[token] += 1
    lines += 1
    if lines % 1000 == 0: sys.stderr.write(".")
    if lines % 50000 == 0:  sys.stderr.write(" [%d]\n" % lines)
  sys.stderr.write("\n")
  counts[extract.BOS] += lines
  counts[extract.EOS] += lines
  LOG.info("Vocabulary size: %d" % len(counts))
  if prune:
    return Counter(dict(counts.most_common(prune)))
  else:
    return counts

def save_vocab(directory, filename, vocab):
  fh = open(directory + "/" + filename, "w")
  for word in vocab:
    print>>fh, word
  
def main():
  logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', datefmt='%Y-%m-%d %H:%M:%S', level=logging.DEBUG)
  parser = optparse.OptionParser("%prog [options]")
  parser.add_option("-e", "--target-language", type="string", dest="target_language")
  parser.add_option("-f", "--source-language", type="string", dest="source_language")
  parser.add_option("-c", "--corpus", type="string", dest="corpus_stem")
  parser.add_option("-t", "--tagged-corpus", type="string", dest="tagged_stem")
  parser.add_option("-a", "--align", type="string", dest="align_file")
  parser.add_option("-w", "--working-dir", type="string", dest="working_dir")
  parser.add_option("-n", "--target-context", type="int", dest="n")
  parser.add_option("-m", "--source-context", type="int", dest="m")
  parser.add_option("-p", "--prune-vocab", type="int", dest="prune")


  parser.set_defaults(
    target_language = "en",
    source_language = "de",
    corpus_stem  = "train.10k",
    align_file = "train.10k.align",
    n = 5,
    m = 4,
    working_dir = "working",
    prune=16000
  )
  options,args = parser.parse_args(sys.argv)

  if not os.path.exists(options.working_dir):
    os.makedirs(options.working_dir)
  else:
    LOG.warn("Directory %s already exists, re-using" % options.working_dir)

  info_file = options.working_dir + "/info"
  if os.path.exists(info_file):
    for line in open(info_file):
      name,value = line[:-1].split()
      if name == "n" and int(value) != options.n or \
          name == "m" and int(value) != options.m:
        LOG.error("info file exists, but parameters do not match. Delete working directory and rerun")
        sys.exit(1)
  else:
    ifh = open(info_file,"w")
    print>>ifh,"m",options.m
    print>>ifh,"n",options.n
    ifh.close()   

  scorpus = options.corpus_stem + "." + options.source_language
  tcorpus = options.corpus_stem + "." + options.target_language

  tvocab,svocab = None,None
  # Extract vocabulary, and prune, if required
  svocab = get_pruned_vocab(scorpus,options.prune)
  tvocab = get_pruned_vocab(tcorpus,options.prune)


  file_stem = os.path.basename(options.corpus_stem)
  ngram_file = options.working_dir + "/" + file_stem + ".ngrams"
  ofh = open(ngram_file, "w")
  
  tags = extract.get_ngrams(options.corpus_stem,
                     options.align_file,
                     options.tagged_stem,
                     svocab,
                     tvocab,
                     options.source_language,
                     options.target_language,
                     options.m,
                     options.n,
                     ofh)

  # Save vocabularies
  save_vocab(options.working_dir, "vocab.source", [item[0] for item in svocab.most_common()])
  save_vocab(options.working_dir, "vocab.target", [item[0] for item in tvocab.most_common()])
  vocab = svocab + tvocab
  vocab.update(tags)
  # UNK is always the first vocabulary element. Make sure
  # it appears in position 0
  # We need to use <null> token in the chart decoder in order
  # to correctly estimate the probabilities of incomplete subphrases
  # that are not sentence initial.
  del vocab["<null>"]
  del vocab["<unk>"]
  vocab_list = [item[0] for item in vocab.most_common()]
  vocab_list.insert(0, "<null>")
  vocab_list.insert(0, "<unk>")
  save_vocab(options.working_dir, "vocab", vocab_list)
 
if __name__ == "__main__":
  main()
