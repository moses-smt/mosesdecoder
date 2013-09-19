// $Id: ChartCellCollectionMBOT.h,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include "ChartCellMBOT.h"
#include "WordsRange.h"
#include "ChartCellCollection.h"

namespace Moses
{
class InputType;
class ChartManager;


class ChartCellCollectionMBOT : public ChartCellCollection
{
public:
  typedef std::vector<ChartCellMBOT*> InnerCollType;
  typedef std::vector<InnerCollType> OuterCollType;

protected:
  OuterCollType m_mbotHypoStackColl;

public:
  ChartCellCollectionMBOT(const InputType &input, ChartManager &manager);
  ~ChartCellCollectionMBOT();

  ChartCellMBOT &GetMBOT(const WordsRange &coverage) {
    return *m_mbotHypoStackColl[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
  }

  const ChartCell &GetMBOT(const WordsRange &coverage) const {
    return *m_mbotHypoStackColl[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
  }
};

}

