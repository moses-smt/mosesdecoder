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

#include <vector>
#include "../../moses/src/Phrase.h"

namespace Moses
{
	class ScoreComponentCollection;
}

namespace MosesChart
{

class Hypothesis;

class TrellisNode
{
	friend std::ostream& operator<<(std::ostream&, const TrellisNode&);
public:
	typedef std::vector<TrellisNode*> NodeChildren;

protected:
	const Hypothesis *m_hypo;
	NodeChildren m_edge;

public:
	TrellisNode(const Hypothesis *hypo);
	TrellisNode(const TrellisNode &origNode
						, const TrellisNode &soughtNode
						, const Hypothesis &replacementHypo
						, Moses::ScoreComponentCollection	&scoreChange
						, const TrellisNode *&nodeChanged);
	~TrellisNode();

	const Hypothesis &GetHypothesis() const
	{
		return *m_hypo;
	}
	
	const NodeChildren &GetChildren() const
	{ return m_edge; }

	const TrellisNode &GetChild(size_t ind) const
	{ return *m_edge[ind]; }

	Moses::Phrase GetOutputPhrase() const;
};

}
