#!/usr/bin/env python

import gzip
import os
import re
import numpy as np
import sys

from bleu import BleuScorer
from coll import OrderedDict
# Edit to set moses python path
sys.path.append(os.path.dirname(__file__) + "/../python")
import moses.dictree as binpt

class DataFormatException(Exception):
  pass

class Hypothesis:
  def __init__(self,text,fv,segments=False):
    self.alignment = [] #only stored for segmented hypos
    self.tokens = [] #only stored for segmented hypos
    if not segments:
      self.text = text
      # Triples of (source-start, source-end, target-end) where segments end positions
      # are 1 beyond the last token
    else:
      # recover segmentation
      self.tokens = []
      align_re = re.compile("\|(\d+)-(\d+)\|")
      for token in text.split():
        match = align_re.match(token)
        if match:
          self.alignment.append\
            ((int(match.group(1)), 1+int(match.group(2)), len(self.tokens)))
        else:
          self.tokens.append(token)
      self.text = " ".join(self.tokens)
      if not self.alignment:
        raise DataFormatException("Expected segmentation information not found in nbest")

        
    self.fv = np.array(fv)
    self.score = 0

  def __str__(self):
    return "{text=%s fv=%s score=%5.4f}" % (self.text, str(self.fv), self.score)

class NBestList:
  def __init__(self,id):
    self.id = id
    self.hyps = []

# Maps feature ids (short feature names) to their values
_feature_index = {}
def set_feature_start(name,index):
  indexes = _feature_index.get(name, [index,0])
  indexes[0] = index
  _feature_index[name] = indexes

def set_feature_end(name,index):
  indexes = _feature_index.get(name, [0,index])
  indexes[1] = index
  _feature_index[name] = indexes

def get_feature_index(name):
  return _feature_index.get(name, [0,0])

def get_nbests(nbest_file, segments=False):
  """Iterate through nbest lists"""
  if nbest_file.endswith("gz"):
    fh = gzip.GzipFile(nbest_file)
  else:
    fh = open(nbest_file)
  lineno = 0
  nbest = None
  for line in fh:
    fields = line.split(" ||| ")
    if len(fields) != 4:
      raise DataFormatException("nbest(%d): %s" % (lineno,line))
    (id, text, scores, total) = fields
    if nbest and nbest.id != id:
      yield nbest
      nbest = None
    if not nbest:
      nbest = NBestList(id)
    fv = []
    score_name = None
    for score in scores.split():
      if score.endswith(":"): 
        score = score[:-1]
        if score_name:
          set_feature_end(score_name,len(fv))
        score_name = score
        set_feature_start(score_name,len(fv))
      else:
        fv.append(float(score))
    if score_name: set_feature_end(score_name,len(fv))
    hyp = Hypothesis(text[:-1],fv,segments)
    nbest.hyps.append(hyp)
  if nbest:
    yield nbest

def get_scores(score_data_file):
  """Iterate through the score data, returning a set of scores for each sentence"""
  scorer = BleuScorer()
  fh = open(score_data_file)
  lineno = 0
  score_vectors = None
  for line in fh:
    if line.startswith("SCORES_TXT_BEGIN"):
      score_vectors = []
    elif line.startswith("SCORES_TXT_END"):
      scores = [scorer.score(score_vector) for score_vector in score_vectors]
      yield scores
    else:
      score_vectors.append([float(i) for i in line[:-1].split()])
  

def get_scored_nbests(nbest_file, score_data_file, input_file, segments=False):
  score_gen = get_scores(score_data_file)
  input_gen = None
  if input_file: input_gen =  open(input_file)
  try:
    for nbest in get_nbests(nbest_file, segments=segments):
      scores = score_gen.next()
      if len(scores) != len(nbest.hyps):
        raise DataFormatException("Length of nbest %s does not match score list (%d != %d)" %
          (nbest.id,len(nbest.hyps), len(scores)))
      input_line = None
      if input_gen:
        input_line = input_gen.next()[:-1]
      for hyp,score in zip(nbest.hyps, scores):
        hyp.score = score
        hyp.input_line = input_line
      yield nbest
  
  except StopIteration:
    raise DataFormatException("Score file shorter than nbest list file")

class PhraseCache:
  """An LRU cache for ttable lookups"""
  def __init__(self, max_size):
    self.max_size = max_size
    self.pairs_to_scores = OrderedDict()

  def get(self, source, target):
    key = (source,target)
    scores = self.pairs_to_scores.get(key,None)
    if scores:
      # cache hit - update access time
      del self.pairs_to_scores[key]
      self.pairs_to_scores[key] = scores
    return scores

  def add(self,source,target,scores):
    key = (source,target)
    self.pairs_to_scores[key] = scores
    while len(self.pairs_to_scores) > self.max_size:
      self.pairs_to_scores.popitem(last=False)

# 
# Should I store full lists of options, or just phrase pairs?
# Should probably store phrase-pairs, but may want to add
# high scoring pairs (say, 20?) when I load the translations
# of a given phrase
#

class CachedPhraseTable:
  def __init__(self,ttable_file,nscores=5,cache_size=20000):
    wa = False
    if binpt.PhraseDictionaryTree.canLoad(ttable_file,True):
      # assume word alignment is included
      wa = True
    self.ttable = binpt.PhraseDictionaryTree(ttable_file,nscores = nscores,wa = wa, tableLimit=0)
    self.cache = PhraseCache(cache_size)
    self.nscores = nscores

  def get_scores(self,phrase):
    source = " ".join(phrase[0])
    target_tuple = tuple(phrase[1])
    target = " ".join(target_tuple)
    scores = self.cache.get(source,target)
    if not scores:
      # cache miss
      scores = [0] * (self.nscores-1) # ignore penalty
      entries = self.ttable.query(source, converter=None)
      # find correct target
      for entry in entries:
        if entry.rhs  == target_tuple:
          scores = entry.scores[:-1]
          break
      #print "QUERY",source,"|||",target,"|||",scores
      self.cache.add(source,target,scores)
    #else:
    #  print "CACHE",source,"|||",target,"|||",scores
    return scores
 

class MosesPhraseScorer:
  def __init__(self,ttable_files, cache_size=20000):
    self.ttables = []
    for ttable_file in ttable_files:
      self.ttables.append(CachedPhraseTable(ttable_file, cache_size=cache_size))
    
  def add_scores(self, hyp):
    """Add the phrase scores to a hypothesis"""
    # Collect up the phrase pairs
    phrases = []
    source_tokens = hyp.input_line.split()
    tgt_st = 0
    if not hyp.alignment:
      raise DataFormatException("Alignments missing from: " + str(hyp))
    for src_st,src_end,tgt_end in hyp.alignment:
      phrases.append((source_tokens[src_st:src_end], hyp.tokens[tgt_st:tgt_end]))
      tgt_st = tgt_end
    # Look up the scores
    phrase_scores = []
    for ttable in self.ttables:
      phrase_scores.append([])
      for phrase in phrases:
        phrase_scores[-1].append(ttable.get_scores(phrase))
#    phrase_scores = np.array(phrase_scores)
#    eps = np.exp(-100)
#    phrase_scores[phrase_scores<eps]=eps
    floor = np.exp(-100)
    phrase_scores = np.clip(np.array(phrase_scores), floor, np.inf)
    hyp.phrase_scores = phrase_scores



