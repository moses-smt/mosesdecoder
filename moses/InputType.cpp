// $Id$
// vim:tabstop=2

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

#include <cstdlib>

#include "InputType.h"
#include "ChartTranslationOptions.h"
#include "StaticData.h"

namespace Moses
{

InputType::InputType(long translationId) : m_translationId(translationId)
{
  m_frontSpanCoveredLength = 0;
  m_sourceCompleted.resize(0);
}

InputType::~InputType() {}

TO_STRING_BODY(InputType);

std::ostream& operator<<(std::ostream& out,InputType const& x)
{
  x.Print(out);
  return out;
}

// default implementation is one column equals one word
int InputType::ComputeDistortionDistance(const WordsRange& prev, const WordsRange& current) const
{
  int dist = 0;
  if (prev.GetNumWordsCovered() == 0) {
    dist = current.GetStartPos();
  } else {
    dist = (int)prev.GetEndPos() - (int)current.GetStartPos() + 1 ;
  }
  return abs(dist);
}

bool InputType::CanIGetFromAToB(size_t /*start*/, size_t /*end*/) const
{
  return true;
}

std::vector <ChartTranslationOptions*> InputType::GetXmlChartTranslationOptions() const
{
  // default. return nothing
  std::vector <ChartTranslationOptions*> ret;
  return ret;
}

}


