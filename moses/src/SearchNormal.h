
#pragma once

#include <vector>
#include "Search.h"
#include "HypothesisStackNormal.h"
#include "TranslationOptionCollection.h"
#include "Timer.h"

class InputType;
class TranslationOptionCollection;

class SearchNormal: public Search
{
protected:
		const InputType &m_source;
		std::vector < HypothesisStack* > m_hypoStackColl; /**< stacks to store hypotheses (partial translations) */ 
	// no of elements = no of words in source + 1
	TargetPhrase m_initialTargetPhrase; /**< used to seed 1st hypo */
	size_t interrupted_flag;
	HypothesisStackNormal* actual_hypoStack; /**actual (full expanded) stack of hypotheses*/ 
	clock_t m_start; /**< starting time, used for logging */
	const TranslationOptionCollection &m_transOptColl; /**< pre-computed list of translation options for the phrases in this sentence */

	// functions for creating hypotheses
	void ProcessOneHypothesis(const Hypothesis &hypothesis);
	void ExpandAllHypotheses(const Hypothesis &hypothesis,const TranslationOptionList &transOptList);
	void ExpandHypothesis(const Hypothesis &hypothesis,const TranslationOption &transOpt);

public:
	SearchNormal(const InputType &source, const TranslationOptionCollection &transOptColl);
	~SearchNormal();

	void ProcessSentence();

	void OutputHypoStackSize();
	void OutputHypoStack(int stack);

	virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const;
	virtual const Hypothesis *GetBestHypothesis() const;
};

