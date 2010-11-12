// $Id: ChartRule.cpp 3048 2010-04-05 17:25:26Z hieuhoang1972 $
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

#include "ChartRule.h"
#include "TargetPhrase.h"
#include "AlignmentInfo.h"
#include "WordConsumed.h"

using namespace std;

namespace Moses
{

void ChartRule::CreateNonTermIndex()
{
	m_wordsConsumedTargetOrder.resize(m_targetPhrase.GetSize(), NOT_FOUND);
	const AlignmentInfo &alignInfo = m_targetPhrase.GetAlignmentInfo();

	size_t nonTermInd = 0;
	size_t prevSourcePos = 0;
	AlignmentInfo::const_iterator iter;
	for (iter = alignInfo.begin(); iter != alignInfo.end(); ++iter)
	{
		// alignment pairs must be ordered by source index
		size_t sourcePos = iter->first;

		//gaoyang1015: muted the following four lines, the assertion causes trouble when we have word alignment info
		//if (nonTermInd > 0)
		//{
		//	assert(sourcePos > prevSourcePos);
		//}

		prevSourcePos = sourcePos;

		size_t targetPos = iter->second;
		if (m_targetPhrase.GetWord(targetPos).IsNonTerminal()) {//gaoyang1003 add nonterminal check
			m_wordsConsumedTargetOrder[targetPos] = nonTermInd;
			nonTermInd++;
		}//gaoyang1003 add nonterminal check
	}
}

//gaoyang1005
void ChartRule::CreateAlignmentVectorSourceOrder()
{
	//std::cerr << "creating alignmentVectorSourceOrder.\n";

	for (size_t i=0; i<m_sourceSize; i++)
	{
		std::vector<size_t> targetPosVector;
		m_alignmentVectorSourceOrder.push_back(targetPosVector);
	}
	const AlignmentInfo &alignInfo = m_targetPhrase.GetAlignmentInfo();
	AlignmentInfo::const_iterator iter;	
	for (iter = alignInfo.begin(); iter != alignInfo.end(); ++iter)
	{
		size_t sourcePos = iter->first;
		size_t targetPos = iter->second;
		m_alignmentVectorSourceOrder[sourcePos].push_back(targetPos);
	}
	
	/*std::cerr << "checking alingmentVectorSourceOrder.\n";
	for (size_t i=0; i<m_sourceSize; i++)
	{
		std::cerr << "sourcepos: "<<i << std::endl;
		if (IsAligned(i)) 
		{		
			std::cerr << "is aligned\n";
			for (size_t j=0; j<m_alignmentVectorSourceOrder[i].size(); j++)
			{
				std::cerr << "targetpos: "<<m_alignmentVectorSourceOrder[i][j]<<" ";
			}
			std::cerr << "\ngetFirstTargetPosAligned: " << GetFirstTargetPosAlignedWith(i)<<std::endl;
			std::cerr << "getLastTargetPosAligned: " << GetLastTargetPosAlignedWith(i)<<std::endl;
		}
		else std::cerr << "not aligned.\n";
		std::cerr << std::endl;
	}*/

}

/////////////////////////////////////////////////////gaoyang1007: convert absolute pos to pos relative to the local chartrule
const size_t ChartRule::GetSourcePosInRule(size_t posAbsolute) const
{
	//std::cerr << "in GetSourcePosInRule, posAbsolute: "<<posAbsolute<<std::endl;
	//std::cerr << "chartRule: "<< *this << std::endl;
	//std::cerr << "sourcesize is: "<<GetSourceSize()<<std::endl;

	//gaoyang1013: pretty ugly, taking the advantage that glue rules we need to consider here has two components
	size_t posInRule = m_sourceSize-1;//starting from the last pos on the source side of the rule

	for(const WordConsumed *wordConsumed = &m_lastWordConsumed; wordConsumed; wordConsumed = wordConsumed->GetPrevWordsConsumed())
	{
		//std::cerr << "considering wordsRange: "<<wordConsumed->GetWordsRange()<<std::endl;
		//std::cerr << "posInRule: "<<posInRule<<std::endl;
		int diffProduct = (wordConsumed->GetWordsRange().GetStartPos()-posAbsolute)*(wordConsumed->GetWordsRange().GetEndPos()-posAbsolute);
		//std::cerr << "diffProduct: "<<diffProduct<<std::endl;
		if( diffProduct<0 || diffProduct==0)
		{
			//std::cerr << "in range\n";
			return posInRule;
		}
		else 
		{
			//std::cerr << "not in range, consider previous wr"<<std::endl;
			posInRule--;
		}
	}
	return -9999;//gaoyang1007 error return
}
/////////////////////////////////////////////////////////////////////////

//gaoyang1005: check if it is a glue rule with sentence boundary label.
const bool ChartRule::IsGlueRule() const
{
	return m_targetPhrase.GetSourcePhrase()==0;
}//gaoyang1005

//gaoyang1004: check if a source word has aligned target word in m_alignmentVectorSourceOrder, as glue rules are not registered, queries for 
//sentence boundary labels will be false
const bool ChartRule::IsAligned(size_t pos) const
{
	return m_alignmentVectorSourceOrder[pos].size()!=0;
}//gaoyang1004


//gaoyang1004: find the first pos aligned with the source word
const size_t ChartRule::GetFirstTargetPosAlignedWith(size_t pos) const
{
	assert(IsAligned(pos));
	return m_alignmentVectorSourceOrder[pos].front();
}//gaoyang1004

//gaoyang1004: find the last pos aligned with the source word
const size_t ChartRule::GetLastTargetPosAlignedWith(size_t pos) const
{
	assert(IsAligned(pos));
	return m_alignmentVectorSourceOrder[pos].back();
}//gaoyang1004

//gaoyang1015
const std::vector<size_t> ChartRule::GetAlignmentVector(size_t pos) const
{
	return m_alignmentVectorSourceOrder[pos];
}//gaoyang1015

//gaoyang1017
void ChartRule::SetSourceSize()
{
	size_t counter = 0;
	for(const WordConsumed *wordConsumed = &m_lastWordConsumed; wordConsumed; wordConsumed = wordConsumed->GetPrevWordsConsumed())
		counter ++;	
	m_sourceSize = counter;
}//gaoyang1017


//gaoyang1017
const size_t ChartRule::GetSourceSize() const
{
	return m_sourceSize;
}//gaoyang1017


std::ostream& operator<<(std::ostream &out, const ChartRule &rule)
{
	out << rule.m_lastWordConsumed << ": " << rule.m_targetPhrase.GetTargetLHS() << "->" << rule.m_targetPhrase;
	return out;
}

} // namespace

