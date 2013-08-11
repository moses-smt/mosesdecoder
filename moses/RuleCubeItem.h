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

#include "StackVec.h"
#include "TargetPhraseMBOT.h"

#include <vector>
#include <cassert>

namespace Moses
{

class ChartCellCollection;
class ChartHypothesis;
class ChartManager;
class ChartTranslationOptions;
class TargetPhrase;

//Fabienne Braune : l-MBOT target phrase (discontiguous)
class TargetPhraseMBOT;

typedef std::vector<const ChartHypothesis*> HypoList;

/** wrapper around list of target phrase translation options
 * @todo How is this used. Split out into separate source file
 */
class TranslationDimension
{
 public:
  TranslationDimension(std::size_t pos,
                       const std::vector<TargetPhrase*> &orderedTargetPhrases)
    : m_pos(pos)
    , m_orderedTargetPhrases(&orderedTargetPhrases)
  {}

  //Fabienne Braune : Need to know the current position when checking for matching source sides (see RuleCubeMBOT)
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
      	assert(tpConst);
      	std::cerr << *tpConst << std::endl;

      	TargetPhrase * tp = const_cast<TargetPhrase*>(tpConst);
      	assert(tp);

      	TargetPhraseMBOT * tpmbot = dynamic_cast<TargetPhraseMBOT*>(tp);
      	assert(tpmbot);

      	const TargetPhraseMBOT * targetPhrase = const_cast<TargetPhraseMBOT*>(tpmbot);
      	assert(targetPhrase);

      	const PhraseSequence *sequence = targetPhrase->GetMBOTPhrases();
      	assert(sequence);

      	return targetPhrase;
  }

  //Fabienne Braune : check if target phrase matches input parse tree
  //In the current version, the input parse tree is matched when selecting applicable rules ( ChartRuleLookupManager )
  //Here we check during chart parsing if the used rules match the input parse
  bool HasMoreMatchingTargetPhrase() const {
	size_t index_to_check = m_pos;
  	while(index_to_check < m_orderedTargetPhrases->size())
  	{
  	    if(static_cast<TargetPhraseMBOT*>((*m_orderedTargetPhrases)[index_to_check])->isMatchesSource())
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
      	    		if(static_cast<TargetPhraseMBOT*>((*m_orderedTargetPhrases)[index_to_check])->isMatchesSource())
      	    		{
      	    			return index_to_check;
      	    		}
      	    		 index_to_check++;
      	    }
      	    return 0;
       }



  bool operator<(const TranslationDimension &compare) const {
    return GetTargetPhrase() < compare.GetTargetPhrase();
  }

  bool operator==(const TranslationDimension &compare) const {
    return GetTargetPhrase() == compare.GetTargetPhrase();
  }

 private:
  std::size_t m_pos;
  const std::vector<TargetPhrase*> *m_orderedTargetPhrases;
};


/** wrapper around list of hypotheses for a particular non-term of a trans opt
 * @todo How is this used. Split out into separate source file
 */
class HypothesisDimension
{
public:
  HypothesisDimension(std::size_t pos, const HypoList &orderedHypos)
    : m_pos(pos)
    , m_orderedHypos(&orderedHypos)
  {}

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

/** @todo How is this used. Split out into separate source file */
class RuleCubeItem
{
 public:
  RuleCubeItem(const ChartTranslationOptions &, const ChartCellCollection &);
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

  void CreateHypothesis(const ChartTranslationOptions &, ChartManager &);

  ChartHypothesis *ReleaseHypothesis();

  bool operator<(const RuleCubeItem &) const;

 private:
  RuleCubeItem(const RuleCubeItem &);  // Not implemented
  RuleCubeItem &operator=(const RuleCubeItem &);  // Not implemented

  void CreateHypothesisDimensions(const StackVec &);

  TranslationDimension m_translationDimension;
  std::vector<HypothesisDimension> m_hypothesisDimensions;
  ChartHypothesis *m_hypothesis;
  float m_score;
};

}
