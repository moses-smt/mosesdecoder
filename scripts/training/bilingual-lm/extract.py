#!/usr/bin/env python

from collections import Counter
import heapq
import logging
import optparse
import sys

LOG = logging.getLogger(__name__)

BOS = "<s>"
EOS = "</s>"
UNK = "<unk>"

def replace_tags(tokens,tags,vocab):
  for i,t in enumerate(tokens):
    if not t in vocab:
      if i < len(tags):
        tokens[i] = tags[i]
      else:
	print "Error: missing tags for index i:", i
        print ' '.join(tokens)
        print ' '.join(tags)
	tokens[i] = UNK

def replace_unks(tokens,vocab):
  for i,t in enumerate(tokens):
    if not t in vocab:
      tokens[i] = UNK


def get_ngrams(corpus_stem, align_file, tagged_stem, svocab, tvocab, slang,tlang, m, n, ofh):
    """
      m - source context
      n - target context

      returns set of tags used
    """
    tags = Counter()
    sfh = open(corpus_stem + "." + slang)
    tfh = open(corpus_stem + "." + tlang)
    afh = open(align_file)
    fhs = [sfh,tfh,afh]
    if tagged_stem:
      fhs.append(open(tagged_stem + "." + slang))
      fhs.append(open(tagged_stem + "." + tlang))

    count  = 0
    ngrams = 0
    LOG.info("Extracting ngrams")
    for lines  in zip(*fhs):
      stokens = lines[0][:-1].split()
      ttokens = lines[1][:-1].split()
      stokens.append(EOS)
      ttokens.append(EOS)
      if tagged_stem:
        stags = lines[3][:-1].split()
        ttags = lines[4][:-1].split()
        stags.append(EOS)
        ttags.append(EOS)
        tags.update(stags)
        tags.update(ttags)
        replace_tags(stokens,stags,svocab)
        replace_tags(ttokens,ttags,tvocab)
      else:
        replace_unks(stokens,svocab)
        replace_unks(ttokens,tvocab)
      # list aligns for each target
      # Note: align specifies source -> target
      target_aligns = [[] for t in range(len(ttokens))] 
      for atoken in lines[2][:-1].split():
        spos,tpos = atoken.split("-")
        spos,tpos = int(spos), int(tpos)
        target_aligns[tpos].append(spos)
      #EOS alignment
      target_aligns[-1] = [len(stokens)-1]

      for tpos,spos_list in enumerate(target_aligns):
        # Affiliation heuristics - see Devlin t al. p1371
        if not spos_list:
          #tpos has no alignment, look right, then left, then right-right, then left-left etc
          rpos = tpos+1
          lpos = tpos-1
          while rpos < len(ttokens) or lpos >= 0:
            if rpos < len(ttokens) and target_aligns[rpos]:
              spos_list = target_aligns[rpos]
              break
            if lpos >= 0 and target_aligns[lpos]:
              spos_list = target_aligns[lpos]
              break
            rpos += 1
            lpos -= 1

        if not spos_list:
          raise Exception("No alignments in sentence \nSRC: " +  lines[0][:-1] + "\nTGT: " +  lines[1][:-1])
        midpos = (len(spos_list)-1) / 2
        spos = sorted(spos_list)[midpos]


        # source-context, target-context, predicted word
        for i in range(max(0,m-spos)):
          print>>ofh, BOS,
          #print [spos-m/2,spos+m/2+1], stokens[spos-m/2:spos+m/2+1]
        print>>ofh, " ".join([s for s in stokens[max(0,spos-m):spos+m+1]]), 
        for i in range(max(0,spos+m+1-len(stokens))):
          print>>ofh, EOS,
        for i in range(max(0,n-(tpos+1))):
          print>>ofh, BOS,
        print>>ofh, " ".join([t for t in ttokens[max(0,tpos+1-n):tpos+1]]),
        print>>ofh
        ngrams += 1

    
      count += 1
      if count % 1000 == 0: sys.stderr.write(".")
      if count % 50000 == 0:  sys.stderr.write(" [%d]\n" % count)
    ofh.close()
    sys.stderr.write("\n")
    LOG.info("Extracted %d ngrams" % ngrams)
    return tags


