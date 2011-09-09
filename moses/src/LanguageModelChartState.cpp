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

int LanguageModelChartState::Compare(const FFState& o) const
{
  const LanguageModelChartState &other =
      dynamic_cast<const LanguageModelChartState &>( o );

  // prefix
  if (m_hypo->GetCurrSourceRange().GetStartPos() > 0) // not for "<s> ..."
  {
    int ret = m_hypo->GetPrefix().Compare(other.GetHypothesis()->GetPrefix());
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
