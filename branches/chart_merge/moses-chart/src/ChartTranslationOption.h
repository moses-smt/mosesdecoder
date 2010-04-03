#pragma once

#include <iostream>
#include <vector>
#include "../../moses/src/TargetPhrase.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/ChartRule.h"

namespace MosesChart
{

class TranslationOption
{
	friend std::ostream& operator<<(std::ostream& out, const TranslationOption &transOpt);

protected:
	const Moses::ChartRule	&m_rule; /*< output phrase when using this translation option */
	const Moses::WordsRange	&m_wordsRange;
public:
	TranslationOption(const Moses::WordsRange &wordsRange
									, const Moses::ChartRule &chartRule);

	~TranslationOption();

	const Moses::ChartRule &GetChartRule() const
	{ return m_rule; }
	const Moses::WordsRange &GetSourceWordsRange() const
	{ return m_wordsRange; }

	/** return estimate of total cost of this option */
	inline size_t GetArity() const 	 
	{ 	 
		return m_rule.GetTargetPhrase().GetArity(); 	 
	}

	/** return estimate of total cost of this option */
	inline float GetTotalScore() const 	 
	{ 	 
		return m_rule.GetTargetPhrase().GetFutureScore(); 	 
	}

};

}
