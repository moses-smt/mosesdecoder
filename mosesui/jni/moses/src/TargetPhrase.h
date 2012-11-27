// $Id: TargetPhrase.h 3394 2010-08-10 13:12:00Z bhaddow $

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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_PROTOBUF
#include "rule.pb.h"
#endif

namespace Moses
{

class LMList;
class PhraseDictionary;
class GenerationDictionary;
class ScoreProducer;
class TranslationSystem;
class WordPenaltyProducer;

class CountInfo
	{
	public:
		CountInfo()
		{}
		CountInfo(float countSource, float countTarget)
		:m_countSource(countSource)
		,m_countTarget(countTarget)
		{	}
		
		float m_countSource;
		float m_countTarget;
		
	};

	
/** represents an entry on the target side of a phrase table (scores, translation, alignment)
 */
class TargetPhrase: public Phrase
{
	friend std::ostream& operator<<(std::ostream&, const TargetPhrase&);
protected:
	float m_transScore, m_ngramScore, m_fullScore;
	//float m_ngramScore, m_fullScore;
	ScoreComponentCollection m_scoreBreakdown;
	AlignmentInfo m_alignmentInfo;

	// in case of confusion net, ptr to source phrase
	Phrase const* m_sourcePhrase; 
	Word m_lhsTarget;
	CountInfo m_countInfo;

	static bool wordalignflag;
	static bool printalign;
	
public:
	TargetPhrase(FactorDirection direction=Output);
	TargetPhrase(FactorDirection direction, std::string out_string);
	~TargetPhrase();
	/** used by the unknown word handler.
		* Set alignment to 0
		*/
	void SetAlignment();

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

/*  inline float GetTranslationScore() const
  {
    return m_transScore;
  }*/
  /***
   * return the estimated score resulting from our being added to a sentence
   * (it's an estimate because we don't have full n-gram info for the language model
   *  without using the (unknown) full sentence)
   * 
   */
  inline float GetFutureScore() const
  {
    return m_fullScore;
  }
	inline const ScoreComponentCollection &GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}

	//! TODO - why is this needed and is it set correctly by every phrase dictionary class ? should be set in constructor
	void SetSourcePhrase(Phrase const* p) 
	{
		m_sourcePhrase=p;
	}
	Phrase const* GetSourcePhrase() const 
	{
		return m_sourcePhrase;
	}
	
	void SetTargetLHS(const Word &lhs)
	{ 	m_lhsTarget = lhs; }
	const Word &GetTargetLHS() const
	{ return m_lhsTarget; }
	
	void SetAlignmentInfo(const std::string &alignString);
	void SetAlignmentInfo(const std::list<std::pair<size_t,size_t> > &alignmentInfo);
	
	AlignmentInfo &GetAlignmentInfo()
	{ return m_alignmentInfo; }
	const AlignmentInfo &GetAlignmentInfo() const
	{ return m_alignmentInfo; }
	
	void UseWordAlignment(bool a){
		wordalignflag=a;
	};
	bool UseWordAlignment() const {
		return wordalignflag;
	};
	void PrintAlignmentInfo(bool a) {
		printalign=a; 
	}
	bool PrintAlignmentInfo() const {
		return printalign;
	}

	void CreateCountInfo(const std::string &countStr);
	
	TO_STRING();
};

std::ostream& operator<<(std::ostream&, const TargetPhrase&);

}

#endif
