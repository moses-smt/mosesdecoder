#include "Optimiser.h"

using namespace std;


namespace Mira {

  float MiraOptimiser::updateWeights(const std::vector<float>& currWeights,
			  const std::vector<const Moses::ScoreComponentCollection*>& scores,
			  const std::vector<float>& losses,
			  const Moses::ScoreComponentCollection oracleScores,
			  std::vector<float>& newWeights) {

	newWeights = currWeights;

	for(unsigned analyseSentence = 0; analyseSentence < scores.size(); analyseSentence++) {

          float scoreChange = 0.0;
          float norm = 0.0;
          
          for(unsigned score = 0; score < scores[analyseSentence]->size(); score++) {
            float currentScoreChange = oracleScores[score] - (*scores[analyseSentence])[score];
            scoreChange += currentScoreChange * newWeights[score];
            norm += currentScoreChange * currentScoreChange;
          }
        
          float delta;
          if(norm == 0.0) //just in case... :-)
            delta = 0.0;
          else {
            delta = (losses[analyseSentence] - scoreChange) / norm;
            cout << "scoreChange: " << scoreChange
                 << "\ndelta: " << delta
                 << "\nloss: " << losses[analyseSentence] << endl;
          }
          //now get in shape
          if(delta > upperBound_)
            delta = upperBound_;
          else if(delta < lowerBound_)
            delta = lowerBound_;

	  for(unsigned score = 0; score < scores[analyseSentence]->size(); score++) {
	    newWeights[score] += (delta * (oracleScores[score] - (*scores[analyseSentence])[score] ) ); 
	  }

          //calculate max. for criterioin
 	  float sumWeightedFeatures = 0.0;
          for(unsigned score = 0; score < scores[analyseSentence]->size(); score++) {
            sumWeightedFeatures += oracleScores[score]*newWeights[score];
          }

	  if((losses[analyseSentence] - sumWeightedFeatures) > maxTranslation_) {
            maxTranslation_ = losses[analyseSentence] - sumWeightedFeatures;
	  } 
        }

	return maxTranslation_;
  }
 /*
  void MiraOptimiser::updateWeights(const Moses::ScoreComponentCollection& currWeights,
                         const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
                         const std::vector<std::vector<float> >& losses,
                         const Moses::ScoreComponentCollection oracleScores,
                         Moses::ScoreComponentCollection& newWeights) {

	
	newWeights.Assign(currWeights);

	for(unsigned pass = 0; pass < passes_; pass++) {
	  for(unsigned nBestList = 0; nBestList < scores.size(); nBestList++) {
	    for(unsigned analyseSentence = 0; analyseSentence < scores[nBestList].size(); analyseSentence++) {
	      
	      //Moses::ScoreComponentCollection currentScoreChange(oracleScores);
	      //currentScoreChange.MinusEquals(scores[nBestList][analyseSentence]);

	      //float norm = currentScoreChange.InnerProduct(currentScoreChange);

	      //float innerProd = scoreChange.
 
	      //update weights
	      //oracleScores
              //newWeights.PlusEquals(delta * (oracleScores[score] - scores[nBestList][analyseSentence][score]
	      //for(unsigned score = 0; score < scores[nBestList][analyseSentence].size(); score++) {
	      //newWeights[score] +i (delta * (oracleScores[score] - scores[nBestList][analyseSentence][score] ) );	
              //}
            }	
       	  }	
	}

  }
  */
}
