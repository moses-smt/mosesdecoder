#ifndef moses_DiscriminativeReordering_h
#define moses_DiscriminativeReordering_h

#include "FeatureFunction.h"

namespace Moses
{

/** Doesn't do anything but provide a key into the global
 * score array to store the discriminative reordering score in.
 */
class DiscriminativeReordering : public StatelessFeatureFunction {
public:
	float m_score;
	DiscriminativeReordering(ScoreIndexManager &scoreIndexManager);

	size_t GetNumScoreComponents() const;
	std::string GetScoreProducerDescription() const;
	std::string GetScoreProducerWeightShortName() const;
	size_t GetNumInputScores() const;

	//virtual void Evaluate(ScoreComponentCollection* out) const;

};

}

#endif
