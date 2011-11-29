// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <n_gram.h>
#include <lmtable.h>

#include "phrasetable.h"
#include "phraselm.h"

void PhraseLanguageModel::attach(PhraseInfoList &pilist)
{
  phrase_info_list_ = &pilist;
  score_idx_ = pilist.register_data(1);
}

void PhraseLanguageModel::compute_statistic()
{
  compute_lmscores(*phrase_info_list_, false);
}

void PhraseLanguageModel::compute_lmscores(PhraseInfoList &phrase_info_list, bool closed_world)
{
  lmtable lm;
  std::ifstream lmstream(lmfile_.c_str());
  lm.load(lmstream, lmfile_.c_str(), NULL, 0);
  lm.setlogOOVpenalty(10000000);

  assert(!computation_done_);

  Score marginal_score = .0;
  for(PhraseInfoList::iterator it = phrase_info_list.begin(); it != phrase_info_list.end(); ++it) {
    PhraseInfo &pi = *it;
    ngram ng(lm.getDict());
    Score lmscore = 0;
    for(PhraseText::const_string_iterator it = pi.get_phrase().string_begin(); it != pi.get_phrase().string_end(); it++) {
      ng.pushw(it->c_str());
      lmscore += lm.clprob(ng);
    }

    pi.data(score_idx_) = exp10(lmscore);
    marginal_score += pi.data(score_idx_);
  }

  if(closed_world)
    for(PhraseInfoList::iterator it = phrase_info_list.begin(); it != phrase_info_list.end(); ++it) {
      PhraseInfo &pi = *it;
      pi.data(score_idx_) /= marginal_score;
    }

  computation_done_ = true;
}

void ClosedPhraseLanguageModel::compute_statistic()
{
  compute_lmscores(*phrase_info_list_, true);
}
