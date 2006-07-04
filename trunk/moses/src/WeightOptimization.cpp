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

#include "WeightOptimization.h"
#include "StaticData.h"

using namespace std;

WeightOptimizationImpl::WeightOptimizationImpl(StaticData &staticData)
:m_staticData(staticData)
{
}

void WeightOptimizationImpl::SetWeight(const std::vector<float> &weight)
{
	m_staticData.SetWeightDistortion(weight[0]);

	size_t currIndex = 1;

	// LM
	{
		size_t numLM = m_staticData.GetLMSize();
		std::vector<float> lmWeight(numLM);
		for (size_t i = 0 ; i < numLM ; i++)
		{
			lmWeight[i] = weight[currIndex++];
		}
		m_staticData.SetWeightLM(lmWeight);
	}

	// phrase trans
	{
		size_t numPhraseDict = NUM_PHRASE_SCORES * m_staticData.GetPhraseDictionarySize();
		std::vector<float> transModelWeight(numPhraseDict);

		for (size_t i = 0 ; i < numPhraseDict ; i++)
		{
			transModelWeight[i] = weight[currIndex++];
		}
		m_staticData.SetWeightTransModel(transModelWeight);
	}

	// WP
	m_staticData.SetWeightWordPenalty(weight[currIndex++]);

	// generation
	{
		size_t numGenerationDict = m_staticData.GetGenerationDictionarySize();
		std::vector<float> generationWeight(numGenerationDict);

		for (size_t i = 0 ; i < numGenerationDict ; i++)
		{
			generationWeight[i] = weight[currIndex++];
		}
		m_staticData.SetWeightGeneration(generationWeight);
	}
}
