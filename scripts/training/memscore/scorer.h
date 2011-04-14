// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#ifndef SCORER_H
#define SCORER_H

#include "memscore.h"

class PhraseScorerFactory
{
private:
  PhraseTable &phrase_table_;

public:
  explicit PhraseScorerFactory(PhraseTable &phrase_table) :
    phrase_table_(phrase_table) {}

  PhraseScorer *create_scorer(const char *argv[], int &argp, bool reverse);

  PhraseTable &get_phrase_table() const {
    return phrase_table_;
  }

  static const std::vector<String> &scorer_list();
};

class PhraseScorer
{
protected:
  PhraseTable &phrase_table_;
  bool reverse_;

  explicit PhraseScorer(PhraseTable &pt, bool reverse) :
    phrase_table_(!reverse ? pt : pt.reverse()), reverse_(reverse) {}

  PhraseTable::iterator get_pair(Phrase src, Phrase tgt) {
    PhraseTable::iterator it = phrase_table_.find(std::make_pair(src, tgt));
    assert(it != phrase_table_.end());
    return it;
  }

private:
  virtual void do_score_phrases() {}

  virtual Score do_get_score(const PhraseTable::const_iterator &it) = 0;

public:
  virtual ~PhraseScorer() {}

  virtual Score get_discount() {}

  void score_phrases() {
    do_score_phrases();
  }

  Score get_score(const PhrasePairCounts::const_iterator &it) {
    return do_get_score(phrase_table_.find(it));
  }

  Score get_score(const PhraseTable::const_iterator &it) {
    return do_get_score(it);
  }

  Score get_score(Phrase src, Phrase tgt) {
    PhraseTable::const_iterator it = get_pair(src, tgt);
    return do_get_score(it);
  }
};

#endif
