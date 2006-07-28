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

#include <cstring> // memset

#include "LatticeEdge.h"
#include "LanguageModel.h"
#include "LMList.h"
#include "Hypothesis.h"

using namespace std;

LatticeEdge::LatticeEdge(FactorDirection direction, const Hypothesis *prevHypo)
	:m_prevHypo(prevHypo)
	,m_targetPhrase(direction)
#ifdef N_BEST
	,m_scoreBreakdown						(prevHypo->m_scoreBreakdown)
#endif
{}

LatticeEdge::~LatticeEdge()
{
}

void LatticeEdge::ResetScore()
{
#ifdef N_BEST
	m_scoreBreakdown.ZeroAll();
#endif
  std::memset(m_score, 0, sizeof(float) * NUM_SCORES);
}

TO_STRING_BODY(LatticeEdge);

