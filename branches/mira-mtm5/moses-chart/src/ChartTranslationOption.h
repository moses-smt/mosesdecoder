// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
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
