// $Id$

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

#include "FFState.h"

namespace Moses
{

class ChartHypothesis;
class Phrase;
  
//! Abstract class for lexical reordering model states
class LanguageModelChartState : public FFState
{
private:
	float m_prefixScore;
	FFState* m_lmRightContext;
	const ChartHypothesis *m_hypo;

public:
	LanguageModelChartState(float prefixScore, 
													FFState *lmRightContext,
													const ChartHypothesis &hypo)
		:m_prefixScore(prefixScore)
		,m_lmRightContext(lmRightContext)
		,m_hypo(&hypo) 
	{}
	~LanguageModelChartState() {
    delete m_lmRightContext;
  }

	float GetPrefixScore() const { return m_prefixScore; }
	FFState* GetRightContext() const { return m_lmRightContext; }
	const ChartHypothesis* GetHypothesis() const { return m_hypo; }

  int Compare(const FFState& o) const ;

};

}
