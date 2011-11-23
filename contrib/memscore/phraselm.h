// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#ifndef PHRASELM_H
#define PHRASELM_H

#include <cassert>

#include "memscore.h"
#include "phrasetable.h"
#include "statistic.h"

class lmtable;

class PhraseLanguageModel : public PhraseStatistic
{
protected:
  String lmfile_;
  Count score_idx_;

  PhraseInfoList *phrase_info_list_;

  void compute_lmscores(PhraseInfoList &phrase_info_list, bool closed_world);

public:
  PhraseLanguageModel(String lmfile) : lmfile_(lmfile) {}

  virtual void attach(PhraseInfoList &pilist);
  virtual void compute_statistic();

  virtual Score get_score(PhraseInfo &pi) {
    assert(computation_done_);
    return pi.data(score_idx_);
  }
};

class ClosedPhraseLanguageModel : public PhraseLanguageModel
{
public:
  ClosedPhraseLanguageModel(String lmfile) : PhraseLanguageModel(lmfile) {}
  virtual void compute_statistic();
};

#endif
