// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#ifndef STATISTIC_H
#define STATISTIC_H

#include "memscore.h"

class PhraseInfoList;

class PhraseStatistic
{
protected:
  bool computation_done_;

public:
  PhraseStatistic() : computation_done_(false) {}
  virtual ~PhraseStatistic() {}

  virtual void attach(PhraseInfoList &pilist) = 0;
  virtual void compute_statistic() = 0;
  virtual Score get_score(PhraseInfo &pi) = 0;
};

#endif
