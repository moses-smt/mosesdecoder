#include "Optimiser.h"
#include "Hildreth.h"
#include "StaticData.h"

using namespace Moses;
using namespace std;

namespace Optimizer {

size_t MiraOptimiser::updateWeights(
	ScoreComponentCollection& weightUpdate,
	const ScoreProducer* sp,
    const vector<vector<ScoreComponentCollection> >& featureValues,
    const vector<vector<float> >& losses,
    const vector<vector<float> >& bleuScores,
    const vector<vector<float> >& modelScores,
    const vector<ScoreComponentCollection>& oracleFeatureValues,
    const vector<float> oracleBleuScores,
    const vector<float> oracleModelScores,
    float learning_rate) {

	// vector of feature values differences for all created constraints
	vector<ScoreComponentCollection> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> all_losses;

	// most violated constraint in batch
	ScoreComponentCollection max_batch_featureValueDiff;

	// Make constraints for new hypothesis translations
	float epsilon = 0.0001;
	int violatedConstraintsBefore = 0;
	float oldDistanceFromOptimum = 0;
	// iterate over input sentences (1 (online) or more (batch))
	for (size_t i = 0; i < featureValues.size(); ++i) {
		//size_t sentenceId = sentenceIds[i];
		// iterate over hypothesis translations for one input sentence
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			ScoreComponentCollection featureValueDiff = oracleFeatureValues[i];
			featureValueDiff.MinusEquals(featureValues[i][j]);

			if (featureValueDiff.GetL1Norm() == 0) {
				continue;
			}

			float loss = losses[i][j];

		  	// check if constraint is violated
		    bool violated = false;
//		    float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
		    float modelScoreDiff = oracleModelScores[i] - modelScores[i][j];
		    float diff = 0;

		    if (loss > modelScoreDiff)
		    	diff = loss - modelScoreDiff;
		    cerr<<"Loss ("<<loss<<") - modelScoreDiff ("<<modelScoreDiff<<") = "<<diff<<"\n";
		    if (diff > epsilon)
		    	violated = true;
		    if (m_normaliseMargin) {
		      modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
		      loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
		      diff = 0;
		      if (loss > modelScoreDiff) {
			diff = loss - modelScoreDiff;
		      }
		    }

		    if (m_scale_margin) {
		      diff *= oracleBleuScores[i];
		    }

		    featureValueDiffs.push_back(featureValueDiff);
		    lossMinusModelScoreDiffs.push_back(diff);
		    all_losses.push_back(loss);
		    if (violated) {
		      ++violatedConstraintsBefore;
		      oldDistanceFromOptimum += diff;
		    }
		}
	}

	// run optimisation: compute alphas for all given constraints
	vector<float> alphas;
	ScoreComponentCollection summedUpdate;
	if (violatedConstraintsBefore > 0) {
		cerr<<"Features values diff size : "<<featureValueDiffs.size() << " (of which violated: " << violatedConstraintsBefore << ")" << endl;
	  if (m_slack != 0) {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
	  } else {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs);
	  }

	  // Update the weight vector according to the alphas and the feature value differences
	  // * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
	  for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
	  	float alpha = alphas[k];
	  	ScoreComponentCollection update(featureValueDiffs[k]);
	    update.MultiplyEquals(alpha);

	    // sum updates
	    summedUpdate.PlusEquals(update);
	  }
	}
	else {
		return 1;
	}

	// apply learning rate
	if (learning_rate != 1) {
		summedUpdate.MultiplyEquals(learning_rate);
	}

	// scale update by BLEU of oracle (for batch size 1 only)
	if (oracleBleuScores.size() == 1) {
		if (m_scale_update) {
			summedUpdate.MultiplyEquals(oracleBleuScores[0]);
		}
	}
	weightUpdate.PlusEquals(sp,summedUpdate);

	return 0;
}


}

