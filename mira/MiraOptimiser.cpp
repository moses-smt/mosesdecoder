#include "Optimiser.h"

using namespace Moses;
using namespace std;

namespace Mira {
	void MiraOptimiser::updateWeights(Moses::ScoreComponentCollection& weights,
                         	const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
                         	const std::vector< std::vector<float> >& losses,
                         	const Moses::ScoreComponentCollection& oracleScores) {

	for(unsigned batch = 0; batch < scores.size(); batch++) {
	  for(unsigned analyseSentence = 0; analyseSentence < scores[batch].size(); analyseSentence++) {

            float scoreChange = 0.0;
            float norm = 0.0;
          
            for(unsigned score = 0; score < scores[batch][analyseSentence].size(); score++) {
              float currentScoreChange = oracleScores[score] - scores[batch][analyseSentence][score];
              scoreChange += currentScoreChange * weights[score];
              norm += currentScoreChange * currentScoreChange;
            }
        
            float delta;
            if(norm == 0.0) //just in case... :-)
              delta = 0.0;
            else {
              delta = (losses[batch][analyseSentence] - scoreChange) / norm;
              cout << "scoreChange: " << scoreChange
                   << "\ndelta: " << delta
                   << "\nloss: " << losses[batch][analyseSentence] << endl;
            }
            //now get in shape
            if(delta > upperBound_)
              delta = upperBound_;
            else if(delta < lowerBound_)
              delta = lowerBound_;

    	    for(unsigned score = 0; score < scores[analyseSentence].size(); score++) {
	      weights[score] += (delta * (oracleScores[score] - scores[batch][analyseSentence][score]) ); 
	    }
  	    StaticData::GetInstanceNonConst().SetWeightsScoreComponentCollection(weights);	
            //calculate max. for criterion
            /*
 	    float sumWeightedFeatures = 0.0;
            for(unsigned score = 0; score < scores[analyseSentence]->size(); score++) {
              sumWeightedFeatures += oracleScores[score]*newWeights[score];
            }

	    if((losses[analyseSentence] - sumWeightedFeatures) > maxTranslation_) {
              maxTranslation_ = losses[analyseSentence] - sumWeightedFeatures;
	    } 
	    */
          }
	}
  }
}

