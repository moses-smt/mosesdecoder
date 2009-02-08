
#pragma once

#include <vector>
#include "../../moses/src/Util.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/ScoreComponentCollection.h"
#include "../../moses/src/Phrase.h"
#include "../../moses/src/TargetPhrase.h"

namespace MosesChart
{

class QueueEntry;
class Hypothesis;

typedef std::vector<Hypothesis*> ArcList;

class Hypothesis
{
	friend std::ostream& operator<<(std::ostream&, const Hypothesis&);

protected:
	static unsigned int s_HypothesesCreated;

	int m_id; /**< numeric ID of this hypothesis, used for logging */
	const Moses::TargetPhrase	&m_targetPhrase; /**< target phrase being created at the current decoding step */
	Moses::Phrase m_contextPrefix, m_contextSuffix;
	std::vector<size_t> m_wordsConsumedTargetOrder;
	Moses::WordsRange					m_currSourceWordsRange;
	Moses::ScoreComponentCollection m_scoreBreakdown; /*! detailed score break-down by components (for instance language model, word penalty, etc) */
	float m_totalScore;

	ArcList 					*m_arcList; /*! all arcs that end at the same trellis point as this hypothesis */

	std::vector<const Hypothesis*> m_prevHypos;

	size_t GetPrefix(Moses::Phrase &ret, size_t size) const;
	size_t GetSuffix(Moses::Phrase &ret, size_t size) const;

	void CalcLMScore(float &retFullScore, float &retNGramScore);

public:
	Hypothesis(const QueueEntry &queueEntry);
	~Hypothesis();

	int GetId()const
	{	return m_id;}
	const Moses::TargetPhrase &GetCurrTargetPhrase()const
	{ return m_targetPhrase; }
	const Moses::WordsRange &GetCurrSourceRange()const
	{ return m_currSourceWordsRange; }
	inline const ArcList* GetArcList() const
	{
		return m_arcList;
	}

	void CreateOutputPhrase(Moses::Phrase &outPhrase) const;
	Moses::Phrase GetOutputPhrase() const;

	int LMContextCompare(const Hypothesis &other) const;

	const Moses::Phrase &GetPrefix() const
	{ return m_contextPrefix; }
	const Moses::Phrase &GetSuffix() const
	{ return m_contextSuffix; }

	void CalcScore();

	void AddArc(Hypothesis *loserHypo);

	const Moses::ScoreComponentCollection &GetScoreBreakDown() const
	{ return m_scoreBreakdown; }
	float GetTotalScore() const 
	{ return m_totalScore; }

	const std::vector<const Hypothesis*> &GetPrevHypos() const
	{ return m_prevHypos; }

	size_t GetWordsConsumedTargetOrder(size_t pos) const
	{ 
		assert(pos < m_wordsConsumedTargetOrder.size());
		return m_wordsConsumedTargetOrder[pos]; 
	}

	TO_STRING();

};

}

