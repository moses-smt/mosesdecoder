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
