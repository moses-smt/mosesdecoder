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

#include "ChartTranslationOption.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/ChartRule.h"
#include "../../moses/src/WordConsumed.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{

TranslationOption::TranslationOption(const WordsRange &wordsRange
								, const ChartRule &rule)
:m_rule(rule)
,m_wordsRange(wordsRange)
{
	//assert(wordsRange.GetStartPos() == rule.GetWordsConsumed().front()->GetWordsRange().GetStartPos());
	assert(wordsRange.GetEndPos() == rule.GetLastWordConsumed().GetWordsRange().GetEndPos());
}

TranslationOption::~TranslationOption()
{

}

// friend
ostream& operator<<(ostream& out, const TranslationOption& transOpt)
{
	out << transOpt.GetTotalScore()
			<< " " << transOpt.GetTargetPhrase()
			<< " " << transOpt.GetTargetPhrase().GetScoreBreakdown()
			<< " " << transOpt.m_rule;
	
	return out;
}

}

