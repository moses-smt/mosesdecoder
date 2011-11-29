/*
 * File:   lexdecom.h
 * Author: Felipe Sánchez-Martínez, Universitat d'Alacant <fsanchez@dlsi.ua.es>
 *
 * Created on 2010/01/27
 */

#ifndef _LEXDECOM_H
#define	_LEXDECOM_H

#include "phrasetable.h"
#include "scorer.h"

class LexicalDecompositionPhraseScorer : public PhraseScorer
{
private:
  explicit LexicalDecompositionPhraseScorer(PhraseTable &pd, bool reverse, const String &lwfile,
      const char *argv[], int &argp,  const PhraseScorerFactory &ptf);

  virtual void do_score_phrases();
  virtual Score do_get_score(const PhraseTable::const_iterator &it);

  Score get_weight(const String &s_src, const String &s_tgt) const;
  Score get_weight(Count src, Count tgt) const;

  typedef std::map<std::pair<Count,Count>, Score> WeightMapType_;

  WeightMapType_ weight_map_;

  // p(J|I) = probability of source-length J given target-length I
  std::map<unsigned, std::map<unsigned, Score> > prob_srclen_tgtlen_;

  Score get_noisy_or_combination(Count src_word, PhraseInfo &tgt_phrase);

  PhraseScorer* black_box_scorer;

public:
  static PhraseScorer *create_scorer(const char *argv[], int &argp, bool reverse,  const PhraseScorerFactory &ptf);
};

#endif	/* _LEXDECOM_H */
