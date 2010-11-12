#ifndef moses_TreePenaltyProducer_h
#define moses_TreePenaltyProducer_h

#include "FeatureFunction.h"

namespace Moses
{

/** Doesn't do anything but provide a key into the global
 * score array to store the tree penalty in.
 */
class TreePenaltyProducer : public StatelessFeatureFunction {
public:
	float m_score;
	TreePenaltyProducer(ScoreIndexManager &scoreIndexManager);

	size_t GetNumScoreComponents() const;
	std::string GetScoreProducerDescription() const;
	std::string GetScoreProducerWeightShortName() const;
	size_t GetNumInputScores() const;

	//virtual void Evaluate(ScoreComponentCollection* out) const;

};

}

#endif
