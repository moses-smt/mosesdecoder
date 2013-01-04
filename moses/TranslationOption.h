// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_TranslationOption_h
#define moses_TranslationOption_h

#include <map>
#include <vector>
#include <boost/functional/hash.hpp>
#include "WordsBitmap.h"
#include "WordsRange.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "Util.h"
#include "TypeDef.h"
#include "ScoreComponentCollection.h"
#include "StaticData.h"

namespace Moses
{

class PhraseDictionary;
class GenerationDictionary;
class LexicalReordering;

/** Available phrase translation for a particular sentence pair.
 * In a multi-factor model, this is expanded from the entries in the
 * translation tables and generation tables (and pruned to the maximum
 * number allowed). By pre-computing the allowable phrase translations,
 * efficient beam search in Manager is possible when expanding instances
 * of the class Hypothesis - the states in the search.
 *
 * A translation option contains source and target phrase, aggregate
 * and details scores (in m_scoreBreakdown), including an estimate
 * how expensive this option will be in search (used to build the
 * future cost matrix.)
 *
 * m_targetPhrase points to a phrase-table entry.
 * The source word range is zero-indexed, so it can't refer to an empty range. The target phrase may be empty.
 */
class TranslationOption
{
  friend std::ostream& operator<<(std::ostream& out, const TranslationOption& possibleTranslation);

protected:

  TargetPhrase 							m_targetPhrase; /*< output phrase when using this translation option */
  const WordsRange		m_sourceWordsRange; /*< word position in the input that are covered by this translation option */
  float               m_futureScore; /*< estimate of total cost when using this translation option, includes language model probabilities */

  //! in TranslationOption, m_scoreBreakdown is not complete.  It cannot,
  //! for example, know the full n-gram score since the length of the
  //! TargetPhrase may be shorter than the n-gram order.  But, if it is
  //! possible to estimate, it is included here.
  ScoreComponentCollection	m_scoreBreakdown;

  typedef std::map<const ScoreProducer *, Scores> _ScoreCacheMap;
  _ScoreCacheMap m_cachedScores;

public:
  /** constructor. Used by initial translation step */
  TranslationOption(const WordsRange &wordsRange
                    , const TargetPhrase &targetPhrase
                    , const InputType &inputType);
  /** constructor. Used to create trans opt from unknown word */
  TranslationOption(const WordsRange &wordsRange
                    , const TargetPhrase &targetPhrase
                    , const InputType &inputType
                    , const UnknownWordPenaltyProducer* uwpProducer);

  /** copy constructor, but change words range. used by caching */
  TranslationOption(const TranslationOption &copy, const WordsRange &sourceWordsRange);


  /** returns true if all feature types in featuresToCheck are compatible between the two phrases */
  bool IsCompatible(const Phrase& phrase, const std::vector<FactorType>& featuresToCheck) const;

  /** used when precomputing (composing) translation options */
  void MergeNewFeatures(const Phrase& phrase, const ScoreComponentCollection& score, const std::vector<FactorType>& featuresToMerge);

  /** returns target phrase */
  inline const TargetPhrase &GetTargetPhrase() const {
    return m_targetPhrase;
  }

  /** returns source word range */
  inline const WordsRange &GetSourceWordsRange() const {
    return m_sourceWordsRange;
  }

  /** returns source phrase */
  const Phrase *GetSourcePhrase() const {
    return &(m_targetPhrase.GetSourcePhrase());
  }

  /** whether source span overlaps with those of a hypothesis */
  bool Overlap(const Hypothesis &hypothesis) const;

  /** return start index of source phrase */
  inline size_t GetStartPos() const {
    return m_sourceWordsRange.GetStartPos();
  }

  /** return end index of source phrase */
  inline size_t GetEndPos() const {
    return m_sourceWordsRange.GetEndPos();
  }

  /** return length of source phrase */
  inline size_t GetSize() const {
    return m_sourceWordsRange.GetEndPos() - m_sourceWordsRange.GetStartPos() + 1;
  }

  /** return estimate of total cost of this option */
  inline float GetFutureScore() const {
    return m_futureScore;
  }

  /** return true if the source phrase translates into nothing */
  inline bool IsDeletionOption() const {
    return m_targetPhrase.GetSize() == 0;
  }

  /** returns detailed component scores */
  inline const ScoreComponentCollection &GetScoreBreakdown() const {
    return m_scoreBreakdown;
  }

  /** returns cached scores */
  inline const Scores *GetCachedScores(const ScoreProducer *scoreProducer) const {
    _ScoreCacheMap::const_iterator it = m_cachedScores.find(scoreProducer);
    if(it == m_cachedScores.end())
      return NULL;
    else
      return &(it->second);
  }

  /** Calculate future score and n-gram score of this trans option, plus the score breakdowns */
  void CalcScore(const TranslationSystem* system);

  void CacheScores(const ScoreProducer &scoreProducer, const Scores &score);

  TO_STRING();
	
	bool operator== (const TranslationOption &rhs) const
	{
    return m_sourceWordsRange == rhs.m_sourceWordsRange &&
      m_targetPhrase == rhs.m_targetPhrase;
	}

};


//XXX: This doesn't look at the alignment. Is this correct?
inline size_t hash_value(const TranslationOption& translationOption) {
  size_t  seed = 0;
  boost::hash_combine(seed, translationOption.GetTargetPhrase());
  boost::hash_combine(seed, translationOption.GetStartPos());
  boost::hash_combine(seed, translationOption.GetEndPos());
  return seed;
}


}

#endif


