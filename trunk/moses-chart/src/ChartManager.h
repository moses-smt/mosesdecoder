
#pragma once

#include <vector>
#include "ChartCell.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/TrellisPathList.h"
#include "../../moses/src/SentenceStats.h"

namespace MosesChart
{

class Hypothesis;
class TrellisPathList;

class Manager
{
protected:
	Moses::InputType const& m_source; /**< source sentence to be translated */
	ChartCellCollection m_hypoStackColl;
	TranslationOptionCollection m_transOptColl; /**< pre-computed list of translation options for the phrases in this sentence */
  std::auto_ptr<Moses::SentenceStats> m_sentenceStats;

public:
	Manager(Moses::InputType const& source);
	~Manager();
	void ProcessSentence();
	const Hypothesis *GetBestHypothesis() const;
	void CalcNBest(size_t count, MosesChart::TrellisPathList &ret,bool onlyDistinct=0) const;

	void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const;

	const Moses::InputType& GetSource() const {return m_source;}   
	
	Moses::SentenceStats& GetSentenceStats() const
  {
    return *m_sentenceStats;
  }
	/***
	 * to be called after processing a sentence (which may consist of more than just calling ProcessSentence() )
	 */
	void CalcDecoderStatistics() const;
  void ResetSentenceStats(const Moses::InputType& source)
  {
    m_sentenceStats = std::auto_ptr<Moses::SentenceStats>(new Moses::SentenceStats(source));
  }
	
};

}

