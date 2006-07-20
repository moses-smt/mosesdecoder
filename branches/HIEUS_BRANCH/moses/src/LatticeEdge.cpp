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

#include "LatticeEdge.h"
#include "LanguageModel.h"
#include "LMList.h"
#include "Hypothesis.h"

using namespace std;

LatticeEdge::LatticeEdge(FactorDirection direction, const Hypothesis *prevHypo)
	:m_prevHypo(prevHypo)
	,m_phrase(direction)
#ifdef N_BEST
	,m_lmScoreComponent 				(prevHypo->GetLMScoreComponent())
	,m_transScoreComponent			(prevHypo->GetTranslationScoreComponent())
	,m_generationScoreComponent	(prevHypo->GetGenerationScoreComponent())
#endif
{}

LatticeEdge::~LatticeEdge()
{
}

void LatticeEdge::ResetScore()
{
	for (size_t i = 0 ; i < NUM_SCORES ; i++)
	{
		m_score[i]	= 0;
	}
}

#ifdef N_BEST

void LatticeEdge::ResizeComponentScore(const LMList &allLM, const list < DecodeStep > &decodeStepList)
{
	// LM
	LMList::const_iterator iter;
	for (iter = allLM.begin() ; iter != allLM.end() ; ++iter)
	{
		LanguageModel *lm = *iter;
		m_lmScoreComponent[lm->GetId()] = 0.0f;
	}

	// trans & gen
	list < DecodeStep >::const_iterator iterDecodeStep;
	for (iterDecodeStep = decodeStepList.begin() ; iterDecodeStep != decodeStepList.end() ; ++iterDecodeStep)
	{
		const DecodeStep &step = *iterDecodeStep;
		switch (step.GetDecodeType())
		{
		case Translate:
		{
			ScoreComponent &transScoreComponent = m_transScoreComponent.Add(&step.GetPhraseDictionary());
			transScoreComponent.Reset();
			break;
		}
		case Generate:
		{
			m_generationScoreComponent[(size_t) &step.GetGenerationDictionary()] = 0.0f;
			break;
		}
		}
	}

	// reset score
	for (size_t i = 0 ; i < m_lmScoreComponent.size() ; i++)
	{
		m_lmScoreComponent[i] = 0;
	}
}

#endif

