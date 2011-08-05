// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#ifndef SCORER_IMPL_H
#define SCORER_IMPL_H

#include "phrasetable.h"
#include "scorer.h"

class MLPhraseScorer : public PhraseScorer
{
private:
  explicit MLPhraseScorer(PhraseTable &pd, bool reverse) :
    PhraseScorer(pd, reverse) {}

  virtual void do_score_phrases();
  virtual Score do_get_score(const PhraseTable::const_iterator &it);

public:
  static PhraseScorer *create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf);
};

class WittenBellPhraseScorer : public PhraseScorer
{
private:
  explicit WittenBellPhraseScorer(PhraseTable &pd, bool reverse) :
    PhraseScorer(pd, reverse) {}

  virtual Score do_get_score(const PhraseTable::const_iterator &it);

public:
  static PhraseScorer *create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf);
};

class AbsoluteDiscountPhraseScorer : public PhraseScorer
{
private:
  Score discount_;

  explicit AbsoluteDiscountPhraseScorer(PhraseTable &pd, bool reverse) :
    PhraseScorer(pd, reverse) {}

  virtual void do_score_phrases();
  virtual Score do_get_score(const PhraseTable::const_iterator &it);

public:
  Score get_discount();
  static PhraseScorer *create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf);
};

class KNDiscount1PhraseScorer : public PhraseScorer
{
private:
  Count total_distinct_;
  Score discount_;
  Count total_count_;

  explicit KNDiscount1PhraseScorer(PhraseTable &pd, bool reverse) :
    PhraseScorer(pd, reverse) {}

  virtual void do_score_phrases();
  virtual Score do_get_score(const PhraseTable::const_iterator &it);

public:
  static PhraseScorer *create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf);
};

class KNDiscount3PhraseScorer : public PhraseScorer
{
private:
  Score discount1_;
  Score discount2_;
  Score discount3plus_;

  Count total_distinct_n1_;
  Count total_distinct_n2_;
  Count total_distinct_n3plus_;

  explicit KNDiscount3PhraseScorer(PhraseTable &pd, bool reverse) :
    PhraseScorer(pd, reverse) {}

  virtual void do_score_phrases();
  virtual Score do_get_score(const PhraseTable::const_iterator &it);

public:
  static PhraseScorer *create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf);
};

class LexicalWeightPhraseScorer : public PhraseScorer
{
private:
  typedef std::map<std::pair<Count,Count>,Score> WeightMapType_;

  WeightMapType_ weight_map_;
  bool overall_max_score_;
  const Count null_word_;

  LexicalWeightPhraseScorer(PhraseTable &pd, bool reverse, const String &weights, bool overall_max = true);

  Score get_weight(const String &s_src, const String &s_tgt) const;
  Score get_weight(Count src, Count tgt) const;

  virtual void do_score_phrases();
  virtual Score do_get_score(const PhraseTable::const_iterator &it);

public:
  static PhraseScorer *create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf);
};

class ConstantPhraseScorer : public PhraseScorer
{
private:
  Score constant_;

  ConstantPhraseScorer(PhraseTable &pt, bool reverse, Score constant) : PhraseScorer(pt, reverse), constant_(constant) {}

  virtual Score do_get_score(const PhraseTable::const_iterator &it);

public:
  static PhraseScorer *create_scorer(const char *argv[], int &argp, bool reverse, const PhraseScorerFactory &ptf);
};

#endif
