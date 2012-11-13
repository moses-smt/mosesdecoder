// $Id$

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
#include <set>

#include "StaticData.h"
#include "LMList.h"
#include "Phrase.h"
#include "ScoreComponentCollection.h"

using namespace std;

namespace Moses
{
LMList::~LMList()
{
}

void LMList::CleanUp()
{
  RemoveAllInColl(m_coll);
}

void LMList::CalcScore(const Phrase &phrase, float &retFullScore, float &retNGramScore, float &retOOVScore,  ScoreComponentCollection* breakdown) const
{
  const_iterator lmIter;
  for (lmIter = begin(); lmIter != end(); ++lmIter) {
    const LanguageModel &lm = **lmIter;
    const float weightLM = lm.GetWeight();
    const float oovWeightLM = lm.GetOOVWeight();

    float fullScore, nGramScore; 
    size_t oovCount;

    // do not process, if factors not defined yet (happens in partial translation options)
    if (!lm.Useable(phrase))
      continue;

    lm.CalcScore(phrase, fullScore, nGramScore, oovCount);

    if (StaticData::Instance().GetLMEnableOOVFeature()) {
      vector<float> scores(2);
      scores[0] = nGramScore;
      scores[1] = oovCount;
      breakdown->Assign(&lm, scores);
      retOOVScore += oovCount * oovWeightLM;
    } else {
      breakdown->Assign(&lm, nGramScore);  // I'm not sure why += doesn't work here- it should be 0.0 right?
    }
    

    retFullScore   += fullScore * weightLM;
    retNGramScore	+= nGramScore * weightLM;
  }
}

void LMList::Add(LanguageModel *lm)
{
	m_coll.push_back(lm);
}

}

