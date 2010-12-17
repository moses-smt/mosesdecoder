// $Id$
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

#include "ChartCell.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/CellCollection.h"

namespace Moses
{
class InputType;
}

namespace MosesChart
{
class Manager;
	
class ChartCellCollection : public Moses::CellCollection
{
public:
	typedef std::vector<ChartCell*> InnerCollType;
	typedef std::vector<InnerCollType> OuterCollType;

protected:
	OuterCollType m_hypoStackColl;

public:
	ChartCellCollection(const Moses::InputType &input, Manager &manager);
	~ChartCellCollection();

	ChartCell &Get(const Moses::WordsRange &coverage)
	{
		return *m_hypoStackColl[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
	}
	const ChartCell &Get(const Moses::WordsRange &coverage) const
	{
		return *m_hypoStackColl[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
	}
	
	const std::vector<Moses::Word> &GetHeadwords(const Moses::WordsRange &coverage) const
	{
		const ChartCell &cell = Get(coverage);
		return cell.GetHeadwords();
	}
};

}

