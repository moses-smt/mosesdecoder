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

#include <vector>
#include "ChartCell.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "../../moses/src/InputType.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/TrellisPathList.h"
#include "../../moses/src/SentenceStats.h"
#include "../../moses/src/TranslationSystem.h"
#include "../../moses/src/ChartRuleLookupManager.h"

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
  const Moses::TranslationSystem* m_system;
	clock_t m_start; /**< starting time, used for logging */
  std::vector<Moses::ChartRuleLookupManager*> m_ruleLookupManagers;
	
public:
  Manager(Moses::InputType const& source, const Moses::TranslationSystem* system);
	~Manager();
	void ProcessSentence();
	const Hypothesis *GetBestHypothesis() const;
	void CalcNBest(size_t count, MosesChart::TrellisPathList &ret,bool onlyDistinct=0) const;

	void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const;

	const Moses::InputType& GetSource() const {return m_source;}
    const Moses::TranslationSystem* GetTranslationSystem() const {return m_system;} 
	
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

