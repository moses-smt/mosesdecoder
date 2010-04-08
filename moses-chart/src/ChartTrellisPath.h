
#pragma once

#include "ChartTrellisNode.h"
#include "../../moses/src/ScoreComponentCollection.h"
#include "../../moses/src/Phrase.h"

namespace MosesChart
{
class Hypothesis;
class TrellisPathCollection;

class TrellisPath
{
	friend std::ostream& operator<<(std::ostream&, const TrellisPath&);

protected:
	// recursively point backwards
	TrellisNode *m_finalNode;
	const TrellisNode *m_prevNodeChanged;
	const TrellisPath *m_prevPath;

	Moses::ScoreComponentCollection	m_scoreBreakdown;
	float m_totalScore;

	// deviate by 1 hypo
	TrellisPath(const TrellisPath &origPath
						, const TrellisNode &soughtNode
						, const Hypothesis &replacementHypo
						, Moses::ScoreComponentCollection	&scoreChange);

	void CreateDeviantPaths(TrellisPathCollection &pathColl, const TrellisNode &soughtNode) const;

	const TrellisNode &GetFinalNode() const
	{ 
		assert (m_finalNode);
		return *m_finalNode;	
	}

public:
	TrellisPath(const Hypothesis *hypo);
	~TrellisPath();

	//! get score for this path throught trellis
	inline float GetTotalScore() const 
	{ return m_totalScore; }

	Moses::Phrase GetOutputPhrase() const;

	/** returns detailed component scores */
	inline const Moses::ScoreComponentCollection &GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}

	void CreateDeviantPaths(TrellisPathCollection &pathColl) const;
};


}

