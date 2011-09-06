//
//  LanguageModelChartState.cpp
//  moses
//
//  Created by Hieu Hoang on 06/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "LanguageModelChartState.h"
#include "ChartHypothesis.h"
#include "ChartManager.h"

namespace Moses
{

KenLMLeftState::KenLMLeftState(const ChartHypothesis &hypo)
  :m_prefix(hypo.GetPrefix())
{
}

int KenLMLeftState::Compare(const KenLMLeftState& other) const 
{
  int ret = m_prefix.Compare(other.m_prefix);
  if (ret != 0)
    return ret;
  
  return 0;
}


int LanguageModelChartState::Compare(const FFState& o) const 
{
  const LanguageModelChartState &other =
  dynamic_cast<const LanguageModelChartState &>( o );
  
  // prefix
  if (m_hypo->GetCurrSourceRange().GetStartPos() > 0) // not for "<s> ..."
  {
    //int ret = m_hypo->GetPrefix().Compare(other.GetHypothesis()->GetPrefix());
    int ret = m_kenLMLeftState.Compare(other.m_kenLMLeftState);
    if (ret != 0)
      return ret;
  }
  
  // suffix
  size_t inputSize = m_hypo->GetManager().GetSource().GetSize();
  if (m_hypo->GetCurrSourceRange().GetEndPos() < inputSize - 1)// not for "... </s>"
  {
    int ret = other.GetRightContext()->Compare( *m_lmRightContext );
    if (ret != 0)
      return ret;
  }
  
  //		size_t inputSize = m_hypo->GetManager().GetSource().GetSize();
  //		if (m_hypo->GetCurrSourceRange().GetEndPos() < inputSize - 1) 
  //		{
  //			int ret2 = m_hypo->GetSuffix().Compare(other.GetHypothesis()->GetSuffix());
  //			if (ret != 0)
  //				return ret;
  //		}
  
  return 0;
}


} // namespace


