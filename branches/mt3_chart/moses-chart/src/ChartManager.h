
#pragma once

#include <vector>
#include "ChartCell.h"
#include "ChartTranslationOptionCollection.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/TrellisPathList.h"

namespace MosesChart
{

class Hypothesis;
class TrellisPathList;

class Manager
{
protected:
	Moses::InputType const& m_source; /**< source sentence to be translated */
	std::map<Moses::WordsRange, ChartCell*> m_hypoStackColl;
	TranslationOptionCollection m_transOptColl; /**< pre-computed list of translation options for the phrases in this sentence */

public:
	Manager(Moses::InputType const& source);
	~Manager();
	void ProcessSentence();
	const Hypothesis *GetBestHypothesis() const;
	void CalcNBest(size_t count, MosesChart::TrellisPathList &ret,bool onlyDistinct=0) const;

	/***
	 * to be called after processing a sentence (which may consist of more than just calling ProcessSentence() )
	 */
	void CalcDecoderStatistics() const;

};

}

