/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <iostream>

#include "TargetPhraseMBOT.h"
#include "ChartHypothesisMBOT.h"

namespace Moses
{

class ChartCellCollection;
class ChartHypothesis;
class ChartHypothesisMBOT;
class ChartManager;
class ChartTranslationOption;
class ChartTranslationOptionMBOT;
class DottedRule;
class TargetPhrase;
class TargetPhraseMBOT;

typedef std::vector<const ChartHypothesis*> HypoList;
typedef std::vector<const ChartHypothesisMBOT*> HypoListMBOT;

// wrapper around list of target phrase translation options
class TranslationDimension
{
 public:
  TranslationDimension(std::size_t pos,
                       const std::vector<TargetPhrase*> &orderedTargetPhrases)
    : m_pos(pos)
    , m_orderedTargetPhrases(&orderedTargetPhrases)
  {
    //const TargetPhraseMBOT * targetPhrase = GetTargetPhraseMBOT();
    //std::cout << "new TranslationDimension() : " << this << " : TP " << targetPhrase << std::endl;
  }

  //For debugging : need to know in which position of the translation dimension I am
  std::size_t GetPosition() const
  {return m_pos;};

  std::size_t IncrementPos() { return m_pos++; }

  bool HasMoreTranslations() const {
    return m_pos+1 < m_orderedTargetPhrases->size();
  }

    const TargetPhrase *GetTargetPhrase() const {
    return (*m_orderedTargetPhrases)[m_pos];
  }

    const TargetPhraseMBOT *GetTargetPhraseMBOT() const {
    	const TargetPhrase * tpConst = (*m_orderedTargetPhrases)[m_pos];
    	TargetPhrase * tp = const_cast<TargetPhrase*>(tpConst);
    	TargetPhraseMBOT * tpmbot = static_cast<TargetPhraseMBOT*>(tp);
    	const TargetPhraseMBOT * targetPhrase = const_cast<TargetPhraseMBOT*>(tpmbot);
    	return targetPhrase;
      }

    //We want to know if there is one more matching target phrase
    bool HasMoreMatchingTargetPhrase() const {
    	//if the target phrase begins with <s> then we know that we are dealing with the very last TOP rule
    	//std::cerr << "LOOKING FOR MATCHING TARGET PHRASE " << std::endl;
		size_t index_to_check = m_pos;
    	while(index_to_check < m_orderedTargetPhrases->size())
    	{
    		//std::cerr << "CHEKCING FOR INDEX : " << index_to_check << m_orderedTargetPhrases->size() << std::endl;

    	    if((*m_orderedTargetPhrases)[index_to_check]->isMatchesSource())
    	    {
    	    	return true;
    	    }
    	    index_to_check++;
    	}
    	return false;
    }

    size_t GetPositionOfMatchingTargetPhrase() const {
        size_t index_to_check = m_pos;
        	    while(index_to_check < m_orderedTargetPhrases->size())
        	    {
        	    		if((*m_orderedTargetPhrases)[index_to_check]->isMatchesSource())
        	    		{
        	    			return index_to_check;
        	    		}
        	    		 index_to_check++;
        	    }
        	    return 0;
         }

  bool operator<(const TranslationDimension &compare) const {
    //std::cout << "Comparing Target Phrases < : "<< std::endl;
    return GetTargetPhrase() < compare.GetTargetPhrase();
  }

  bool operator==(const TranslationDimension &compare) const {
    //std::cout << "Comparing Target Phrases == : "<< std::endl;
    return GetTargetPhrase() == compare.GetTargetPhrase();
  }

 private:
  std::size_t m_pos;
  const std::vector<TargetPhrase*> *m_orderedTargetPhrases;
};


// wrapper around list of hypotheses for a particular non-term of a trans opt
class HypothesisDimension
{
public:
  HypothesisDimension(std::size_t pos, const HypoList &orderedHypos)
    : m_pos(pos)
    , m_orderedHypos(&orderedHypos)
  {
    //std::cout << "new hypothesisDimension()" << std::endl;
  }

//BEWARE : inserted for testing
~HypothesisDimension()
 {
     //std::cout << "DESTROYED DIMENSION"<< std::endl;
 };


  std::size_t IncrementPos() { return m_pos++; }

  bool HasMoreHypo() const {
    return m_pos+1 < m_orderedHypos->size();
  }

  const ChartHypothesis *GetHypothesis() const {
    return (*m_orderedHypos)[m_pos];
  }

  bool operator<(const HypothesisDimension &compare) const {
    return GetHypothesis() < compare.GetHypothesis();
  }

  bool operator==(const HypothesisDimension &compare) const {
    return GetHypothesis() == compare.GetHypothesis();
  }

private:
  std::size_t m_pos;
  const HypoList *m_orderedHypos;
};

std::size_t hash_value(const HypothesisDimension &);

class RuleCubeItem
{
 public:
  RuleCubeItem(const ChartTranslationOption &, const ChartCellCollection &);
  RuleCubeItem(const RuleCubeItem &, int);
  ~RuleCubeItem();

  const TranslationDimension &GetTranslationDimension() const {
    return m_translationDimension;
  }

  const std::vector<HypothesisDimension> &GetHypothesisDimensions() const {
    return m_hypothesisDimensions;
  }

  float GetScore() const { return m_score; }

  void EstimateScore();

  virtual void CreateHypothesis(const ChartTranslationOption &, ChartManager &);

  virtual ChartHypothesis *ReleaseHypothesis();

  virtual bool operator<(const RuleCubeItem &) const;

 private:
  RuleCubeItem(const RuleCubeItem &);  // Not implemented
  RuleCubeItem &operator=(const RuleCubeItem &);  // Not implemented

 private:
  void CreateHypothesisDimensions(const DottedRule &,
                                  const ChartCellCollection &);

  TranslationDimension m_translationDimension;
  std::vector<HypothesisDimension> m_hypothesisDimensions;
  ChartHypothesis *m_hypothesis;
  float m_score;
};
}
