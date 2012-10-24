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

#ifndef moses_TargetPhrase_h
#define moses_TargetPhrase_h

#include <vector>
#include "TypeDef.h"
#include "Phrase.h"
#include "ScoreComponentCollection.h"
#include "AlignmentInfo.h"

#include "util/string_piece.hh"

#ifdef HAVE_PROTOBUF
#include "rule.pb.h"
#endif

namespace Moses
{

class LMList;
class ScoreProducer;
class TranslationSystem;
class WordPenaltyProducer;

/** represents an entry on the target side of a phrase table (scores, translation, alignment)
 */
class TargetPhrase: public Phrase
{
  friend std::ostream& operator<<(std::ostream&, const TargetPhrase&);
protected:
  float m_fullScore;
  ScoreComponentCollection m_scoreBreakdown;

	// in case of confusion net, ptr to source phrase
	Phrase m_sourcePhrase; 
	const AlignmentInfo* m_alignTerm, *m_alignNonTerm;
	Word m_lhsTarget;

public:
  TargetPhrase();
  explicit TargetPhrase(std::string out_string);
  explicit TargetPhrase(const Phrase &targetPhrase);

  //! used by the unknown word handler- these targets
  //! don't have a translation score, so wp is the only thing used
  void SetScore(const TranslationSystem* system);

  //!Set score for Sentence XML target options
  void SetScore(float score);

  //! Set score for unknown words with input weights
  void SetScore(const TranslationSystem* system, const Scores &scoreVector);


  /*** Called immediately after creation to initialize scores.
   *
   * @param translationScoreProducer The PhraseDictionaryMemory that this TargetPhrase is contained by.
   *        Used to identify where the scores for this phrase belong in the list of all scores.
   * @param scoreVector the vector of scores (log probs) associated with this translation
   * @param weighT the weights for the individual scores (t-weights in the .ini file)
   * @param languageModels all the LanguageModels that should be used to compute the LM scores
   * @param weightWP the weight of the word penalty
   *
   * @TODO should this be part of the constructor?  If not, add explanation why not.
  	*/
  void SetScore(const ScoreProducer* translationScoreProducer,
                const Scores &scoreVector,
                const ScoreComponentCollection &sparseScoreVector,
                const std::vector<float> &weightT,
                float weightWP,
                const LMList &languageModels);

  void SetScoreChart(const ScoreProducer* translationScoreProducer
                     ,const Scores &scoreVector
                     ,const std::vector<float> &weightT
                     ,const LMList &languageModels
                     ,const WordPenaltyProducer* wpProducer);

  // used by for unknown word proc in chart decoding
  void SetScore(const ScoreProducer* producer, const Scores &scoreVector);


  // used when creating translations of unknown words:
  void ResetScore();
  void SetWeights(const ScoreProducer*, const std::vector<float> &weightT);

  TargetPhrase *MergeNext(const TargetPhrase &targetPhrase) const;
  // used for translation step

#ifdef HAVE_PROTOBUF
  void WriteToRulePB(hgmert::Rule* pb) const;
#endif

  /***
   * return the estimated score resulting from our being added to a sentence
   * (it's an estimate because we don't have full n-gram info for the language model
   *  without using the (unknown) full sentence)
   *
   */
  inline float GetFutureScore() const {
    return m_fullScore;
  }
  inline void SetFutureScore(float fullScore) {
    m_fullScore = fullScore;
  }
	inline const ScoreComponentCollection &GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}

  //TODO: Probably shouldn't copy this, but otherwise ownership is unclear
	void SetSourcePhrase(const Phrase&  p) 
	{
		m_sourcePhrase=p;
	}
  // ... but if we must store a copy, at least initialize it in-place
  Phrase &MutableSourcePhrase() {
    return m_sourcePhrase;
  }
	const Phrase& GetSourcePhrase() const 
	{
		return m_sourcePhrase;
	}
	
	void SetTargetLHS(const Word &lhs)
	{ 	m_lhsTarget = lhs; }
	const Word &GetTargetLHS() const
	{ return m_lhsTarget; }
	
  Word &MutableTargetLHS() {
    return m_lhsTarget;
  }

  void SetAlignmentInfo(const StringPiece &alignString);
  void SetAlignTerm(const AlignmentInfo *alignTerm) {
    m_alignTerm = alignTerm;
  }
  void SetAlignNonTerm(const AlignmentInfo *alignNonTerm) {
    m_alignNonTerm = alignNonTerm;
  }

  void SetAlignTerm(const AlignmentInfo::CollType &coll);
  void SetAlignNonTerm(const AlignmentInfo::CollType &coll);

  const AlignmentInfo &GetAlignTerm() const
	{ return *m_alignTerm; }
  const AlignmentInfo &GetAlignNonTerm() const
	{ return *m_alignNonTerm; }
	

  TO_STRING();
};

std::ostream& operator<<(std::ostream&, const TargetPhrase&);

/**
 * Hasher that looks at source and target phrase.
 **/
struct TargetPhraseHasher 
{
  inline size_t operator()(const TargetPhrase& targetPhrase) const
  {
    size_t seed = 0;
    boost::hash_combine(seed, targetPhrase);
    boost::hash_combine(seed, targetPhrase.GetSourcePhrase());
    boost::hash_combine(seed, targetPhrase.GetAlignTerm());
    boost::hash_combine(seed, targetPhrase.GetAlignNonTerm());

    return seed;
  }
};

struct TargetPhraseComparator
{
  inline bool operator()(const TargetPhrase& lhs, const TargetPhrase& rhs) const
  {
    return lhs.Compare(rhs) == 0 &&
      lhs.GetSourcePhrase().Compare(rhs.GetSourcePhrase()) == 0 &&
      lhs.GetAlignTerm() == rhs.GetAlignTerm() &&
      lhs.GetAlignNonTerm() == rhs.GetAlignNonTerm();
  }

};

}

#endif
