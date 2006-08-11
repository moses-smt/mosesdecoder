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

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "Phrase.h"
#include "Hypothesis.h"
#include "TypeDef.h" //FactorArray
#include "InputType.h"
#include "Util.h" //Join()
#include "PhraseReference.h"

struct recombinationInfo
{
	recombinationInfo() {} //for std::vector
	recombinationInfo(unsigned int srcWords, float gProb, float bProb) : numSourceWords(srcWords), betterProb(gProb), worseProb(bProb) {}
	
	unsigned int numSourceWords;
	float betterProb, worseProb;
};

/***
 * stats relating to decoder operation on a given sentence
 */
class SentenceStats
{
	public:
	
		/***
		 * to be called before decoding a sentence
		 */
		SentenceStats(const InputType& source) {Initialize(source);}
		void Initialize(const InputType& source)
		{
			m_numHyposPruned = 0;
			m_totalSourceWords = source.GetSize();
			m_recombinationInfos.clear();
			m_deletedWords.clear();
			m_insertedWords.clear();
		}
		
		/***
		 * to be called after decoding a sentence
		 */
		void CalcFinalStats(const Hypothesis& bestHypo);
		
		unsigned int GetTotalHypos() const {return Hypothesis::s_HypothesesCreated;}
		unsigned int GetNumHyposRecombined() const {return m_recombinationInfos.size();}
		unsigned int GetNumHyposPruned() const {return m_numHyposPruned;}
		unsigned int GetTotalSourceWords() const {return m_totalSourceWords;}
		unsigned int GetNumWordsDeleted() const {return m_deletedWords.size();}
		unsigned int GetNumWordsInserted() const {return m_insertedWords.size();}
		const std::vector<PhraseReference>& GetDeletedWords() const {return m_deletedWords;}
		const std::vector<std::string>& GetInsertedWords() const {return m_insertedWords;}
		
		void AddRecombination(const Hypothesis& worseHypo, const Hypothesis& betterHypo)
		{
			m_recombinationInfos.push_back(recombinationInfo(worseHypo.GetWordsBitmap().GetNumWordsCovered(), 
													betterHypo.GetTotalScore(), worseHypo.GetTotalScore()));
		}
		void AddPruning() {m_numHyposPruned++;}
		
	protected:
	
		/***
		 * auxiliary to CalcFinalStats()
		 */
		void SentenceStats::AddDeletedWords(const Hypothesis& hypo);
	
		//hypotheses
		std::vector<recombinationInfo> m_recombinationInfos;
		unsigned int m_numHyposPruned;
	
		//words
		unsigned int m_totalSourceWords;
		std::vector<PhraseReference> m_deletedWords; //count deleted words/phrases in the final hypothesis
		std::vector<std::string> m_insertedWords; //count inserted words in the final hypothesis
};

inline std::ostream& operator<<(std::ostream& os, const SentenceStats& ss)
{
  return os << "total hypotheses generated = " << ss.GetTotalHypos() << std::endl
            << "         number recombined = " << ss.GetNumHyposRecombined() << std::endl
            << "             number pruned = " << ss.GetNumHyposPruned() << std::endl
            << "        total source words = " << ss.GetTotalSourceWords() << std::endl
            << "             words deleted = " << ss.GetNumWordsDeleted() << " (" << Join(" ", ss.GetDeletedWords()) << ")" << std::endl
            << "            words inserted = " << ss.GetNumWordsInserted() << " (" << Join(" ", ss.GetInsertedWords()) << ")" << std::endl;
}
