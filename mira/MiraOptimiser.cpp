#include "Optimiser.h"
#include "Hildreth.h"
#include "StaticData.h"

using namespace Moses;
using namespace std;

namespace Mira {

vector<int> MiraOptimiser::updateWeights(ScoreComponentCollection& currWeights,
    const vector<vector<ScoreComponentCollection> >& featureValues,
    const vector<vector<float> >& losses,
    const vector<vector<float> >& bleuScores,
    const vector<ScoreComponentCollection>& oracleFeatureValues,
    const vector<float> oracleBleuScores,
    const vector<size_t> sentenceIds,
    float learning_rate,
    size_t rank,
    size_t epoch) {

	// vector of feature values differences for all created constraints
	vector<ScoreComponentCollection> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> all_losses;

	// most violated constraint in batch
	ScoreComponentCollection max_batch_featureValueDiff;
	float max_batch_lossMinusModelScoreDiff = -1;

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

			cerr << "Rank " << rank << ", epoch " << epoch << ", feature value diff: " << featureValueDiff << endl;
			if (featureValueDiff.GetL1Norm() == 0) {
				// skip constraint
				continue;
			}

			float loss = losses[i][j];
		    if (m_scale_margin == 1) {
		    	loss *= oracleBleuScores[i];
		    	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", scaling margin with oracle bleu score "  << oracleBleuScores[i] << endl);
		    }
		    else if (m_scale_margin == 2) {
		    	loss *= log2(oracleBleuScores[i]);
		    	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", scaling margin with log2 oracle bleu score "  << log2(oracleBleuScores[i]) << endl);
		    }
		    else if (m_scale_margin == 10) {
		    	loss *= log10(oracleBleuScores[i]);
		    	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", scaling margin with log10 oracle bleu score "  << log10(oracleBleuScores[i]) << endl)
		    }

		  	// check if constraint is violated
		    bool violated = false;
		    bool addConstraint = true;
		    float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
		    float diff = 0;
			if (loss > (modelScoreDiff + m_margin_slack)) {
				diff = loss - (modelScoreDiff + m_margin_slack);
			}
			cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " + " <<  m_margin_slack << " >= " << loss << " (current violation: " << diff << ")" << endl;

		    if (diff > epsilon) {
		    	violated = true;
		    }
		    else if (m_onlyViolatedConstraints) {
		    	addConstraint = false;
			}

		    float lossMinusModelScoreDiff = loss - modelScoreDiff;
		    if (addConstraint) {
		    	featureValueDiffs.push_back(featureValueDiff);
		    	lossMinusModelScoreDiffs.push_back(lossMinusModelScoreDiff);
		    	all_losses.push_back(loss);

		    	if (violated) {
		    		++violatedConstraintsBefore;
		    		oldDistanceFromOptimum += diff;
				}
			}
		}
	}

	// run optimisation: compute alphas for all given constraints
	vector<float> alphas;
	ScoreComponentCollection summedUpdate;
	if (violatedConstraintsBefore > 0) {
	  cerr << "Rank " << rank << ", epoch " << epoch << ", number of constraints passed to optimizer: " <<
			  featureValueDiffs.size() << " (of which violated: " << violatedConstraintsBefore << ")" << endl;
	  if (m_slack != 0) {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
	  } else {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs);
	  }
  
	  // Update the weight vector according to the alphas and the feature value differences
	  // * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
	  for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
	  	float alpha = alphas[k];
	  	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", alpha: " << alpha << endl);
	  	ScoreComponentCollection update(featureValueDiffs[k]);
	    update.MultiplyEquals(alpha);
	    
	    // sum up update
	    summedUpdate.PlusEquals(update);
	  }
	} 
	else {
		cerr << "Rank " << rank << ", epoch " << epoch << ", check, no constraint violated for this batch" << endl;
	  vector<int> status(2);
	  status[0] = 0;
	  status[1] = 0;
	  return status;
	}

	ScoreComponentCollection newWeights(currWeights);
	newWeights.PlusEquals(summedUpdate);

	// Sanity check: are there still violated constraints after optimisation?
	int violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValueDiffs.size(); ++i) {
		float modelScoreDiff = featureValueDiffs[i].InnerProduct(newWeights);
		float loss = all_losses[i];
		float diff = loss - (modelScoreDiff + m_margin_slack);
		if (diff > epsilon) {
			++violatedConstraintsAfter;
			newDistanceFromOptimum += diff;
		}
	}
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", check, violated constraint before: " << violatedConstraintsBefore << ", after: " << violatedConstraintsAfter  << ", change: " << violatedConstraintsBefore - violatedConstraintsAfter << endl);
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", check, error before: " << oldDistanceFromOptimum << ", after: " << newDistanceFromOptimum << ", change: " << oldDistanceFromOptimum - newDistanceFromOptimum << endl);

	// apply learning rate
	if (learning_rate != 1) {
		VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", update before applying learning rate: " << summedUpdate << endl);
		summedUpdate.MultiplyEquals(learning_rate);
		VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", update after applying learning rate: " << summedUpdate << endl);
	}

	// scale update by BLEU of oracle
	if (oracleBleuScores.size() == 1 && m_scale_update) {
		VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", scaling summed update with log10 oracle bleu score " << log10(oracleBleuScores[0]) << endl);
		summedUpdate.MultiplyEquals(log10(oracleBleuScores[0]));
	}

	// apply update to weight vector
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", weights before update: " << currWeights << endl);
	currWeights.PlusEquals(summedUpdate);
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", weights after update: " << currWeights << endl);

	vector<int> status(2);
	status[0] = violatedConstraintsBefore;
	status[1] = violatedConstraintsAfter;
	return status;
}

vector<int> MiraOptimiser::updateWeightsHopeFear(Moses::ScoreComponentCollection& currWeights,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesHope,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesFear,
		const std::vector<std::vector<float> >& bleuScoresHope,
		const std::vector<std::vector<float> >& bleuScoresFear,
		const std::vector< size_t> sentenceIds,
		float learning_rate,
		size_t rank,
		size_t epoch) {

	// vector of feature values differences for all created constraints
	vector<ScoreComponentCollection> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> all_losses;

	// most violated constraint in batch
	ScoreComponentCollection max_batch_featureValueDiff;
	float max_batch_lossMinusModelScoreDiff = -1;

	// Make constraints for new hypothesis translations
	float epsilon = 0.0001;
	int violatedConstraintsBefore = 0;
	float oldDistanceFromOptimum = 0;

	// iterate over input sentences (1 (online) or more (batch))
	for (size_t i = 0; i < featureValuesHope.size(); ++i) {
		size_t sentenceId = sentenceIds[i];							// keep sentenceId for storing more than 1 oracle..
		// Pair all hope translations with all fear translations for one input sentence
		for (size_t j = 0; j < featureValuesHope[i].size(); ++j) {
			for (size_t k = 0; k < featureValuesFear[i].size(); ++k) {
				ScoreComponentCollection featureValueDiff = featureValuesHope[i][j];
				featureValueDiff.MinusEquals(featureValuesFear[i][k]);
				cerr << "Rank " << rank << ", epoch " << epoch << ", feature value diff: " << featureValueDiff << endl;
				if (featureValueDiff.GetL1Norm() == 0) {
					// skip constraint
					continue;
				}

				float loss = bleuScoresHope[i][j] - bleuScoresFear[i][k];
				if (m_scale_margin == 1) {
					loss *= bleuScoresHope[i][j];
					VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", scaling margin with oracle bleu score "  << bleuScoresHope[i][j] << endl);
				}
				else if (m_scale_margin == 2) {
					loss *= log2(bleuScoresHope[i][j]);
					VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", scaling margin with log2 oracle bleu score "  << log2(bleuScoresHope[i][j]) << endl);
				}
				else if (m_scale_margin == 10) {
					loss *= log10(bleuScoresHope[i][j]);
					VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", scaling margin with log10 oracle bleu score "  << log10(bleuScoresHope[i][j]) << endl);
				}

				// check if constraint is violated
				bool violated = false;
				bool addConstraint = true;
				float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
				float diff = 0;
				if (loss > (modelScoreDiff + m_margin_slack)) {
					diff = loss - (modelScoreDiff + m_margin_slack);
				}
				cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " + " << m_margin_slack << " >= " << loss << " (current violation: " << diff << ")" << endl;

				if (diff > epsilon) {
					violated = true;
				}
				else if (m_onlyViolatedConstraints) {
					addConstraint = false;
				}

				float lossMinusModelScoreDiff = loss - modelScoreDiff;
				if (addConstraint) {
					featureValueDiffs.push_back(featureValueDiff);
					lossMinusModelScoreDiffs.push_back(lossMinusModelScoreDiff);
					all_losses.push_back(loss);

					if (violated) {
						++violatedConstraintsBefore;
						oldDistanceFromOptimum += diff;
					}
				}
			}
		}
	}

	// run optimisation: compute alphas for all given constraints
	vector<float> alphas;
	ScoreComponentCollection summedUpdate;
	if (violatedConstraintsBefore > 0) {
	  cerr << "Rank " << rank << ", epoch " << epoch << ", number of constraints passed to optimizer: " <<
			  featureValueDiffs.size() << " (of which violated: " << violatedConstraintsBefore << ")" << endl;
	  if (m_slack != 0) {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
	  } else {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs);
	  }

	  // Update the weight vector according to the alphas and the feature value differences
	  // * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
	  for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
	  	float alpha = alphas[k];
	  	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", alpha: " << alpha << endl);
	  	ScoreComponentCollection update(featureValueDiffs[k]);
	    update.MultiplyEquals(alpha);

	  	// scale update by BLEU of hope translation (only two cases defined at the moment)
	    if (featureValuesHope.size() == 1 && m_scale_update) { // only defined for batch size 1)
	    	if (featureValuesHope[0].size() == 1) {
	    		VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", scaling update with log10 oracle bleu score "  << log10(bleuScoresHope[0][0]) << endl); // only 1 oracle
	    		update.MultiplyEquals(log10(bleuScoresHope[0][0]));
	    	} else if (featureValuesFear[0].size() == 1) {
	    		VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", scaling update with log10 oracle bleu score "  << log10(bleuScoresHope[0][k]) << endl); // k oracles
	    		update.MultiplyEquals(log10(bleuScoresHope[0][k]));
			}
		}

	    // sum up update
	    summedUpdate.PlusEquals(update);
	  }
	}
	else {
		cerr << "Rank " << rank << ", epoch " << epoch << ", check, no constraint violated for this batch" << endl;
	  vector<int> status(2);
	  status[0] = 0;
	  status[1] = 0;
	  return status;
	}

	ScoreComponentCollection newWeights(currWeights);
	newWeights.PlusEquals(summedUpdate);

	// Sanity check: are there still violated constraints after optimisation?
	int violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValueDiffs.size(); ++i) {
		float modelScoreDiff = featureValueDiffs[i].InnerProduct(newWeights);
		float loss = all_losses[i];
		float diff = loss - (modelScoreDiff + m_margin_slack);
		if (diff > epsilon) {
			++violatedConstraintsAfter;
			newDistanceFromOptimum += diff;
		}
	}
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", check, violated constraint before: " << violatedConstraintsBefore << ", after: " << violatedConstraintsAfter  << ", change: " << violatedConstraintsBefore - violatedConstraintsAfter << endl);
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", check, error before: " << oldDistanceFromOptimum << ", after: " << newDistanceFromOptimum << ", change: " << oldDistanceFromOptimum - newDistanceFromOptimum << endl);

	// Apply learning rate (fixed or flexible)
	if (learning_rate != 1) {
		VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", update before applying learning rate: " << summedUpdate << endl);
		summedUpdate.MultiplyEquals(learning_rate);
		VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", update after applying learning rate: " << summedUpdate << endl);
	}

	// apply update to weight vector
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", weights before update: " << currWeights << endl);
	currWeights.PlusEquals(summedUpdate);
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", weights after update: " << currWeights << endl);

	vector<int> statusPlus(2);
	statusPlus[0] = violatedConstraintsBefore;
	statusPlus[1] = violatedConstraintsAfter;
	return statusPlus;
}

vector<int> MiraOptimiser::updateWeightsAnalytically(ScoreComponentCollection& currWeights,
	ScoreComponentCollection& featureValuesHope,
    ScoreComponentCollection& featureValuesFear,
    float bleuScoreHope,
    float bleuScoreFear,
    size_t sentenceId,
    float learning_rate,
    size_t rank,
    size_t epoch) {

  float epsilon = 0.0001;
  float oldDistanceFromOptimum = 0;
  bool constraintViolatedBefore = false;
  ScoreComponentCollection weightUpdate;

 // cerr << "Rank " << rank << ", epoch " << epoch << ", hope: " << featureValuesHope << endl;
 // cerr << "Rank " << rank << ", epoch " << epoch << ", fear: " << featureValuesFear << endl;
  ScoreComponentCollection featureValueDiff = featureValuesHope;
  featureValueDiff.MinusEquals(featureValuesFear);
  cerr << "Rank " << rank << ", epoch " << epoch << ", hope - fear: " << featureValueDiff << endl;
  float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
  float loss = bleuScoreHope - bleuScoreFear;
  float diff = 0;
  if (loss > (modelScoreDiff + m_margin_slack)) {
	  diff = loss - (modelScoreDiff + m_margin_slack);
  }
  cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " + " << m_margin_slack << " >= " << loss << " (current violation: " << diff << ")" << endl;

  if (diff > epsilon) {
    // constraint violated
    oldDistanceFromOptimum += diff;
    constraintViolatedBefore = true;

    // compute alpha for given constraint: (loss - model score diff) / || feature value diff ||^2
    // featureValueDiff.GetL2Norm() * featureValueDiff.GetL2Norm() == featureValueDiff.InnerProduct(featureValueDiff)
    // from Crammer&Singer 2006: alpha = min {C , l_t/ ||x||^2}
    float squaredNorm = featureValueDiff.GetL2Norm() * featureValueDiff.GetL2Norm();

    if (squaredNorm > 0) {
    	float alpha = diff / squaredNorm;
    	if (m_slack > 0 ) {
    		if (alpha > m_slack) {
    			alpha = m_slack;
    		}
    		else if (alpha < m_slack*(-1)) {
    			alpha = m_slack*(-1);
    		}
    	}

    	cerr << "Rank " << rank << ", epoch " << epoch << ", alpha: " << alpha << endl;
    	featureValueDiff.MultiplyEquals(alpha);
    	weightUpdate.PlusEquals(featureValueDiff);
    }
    else {
    	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", no update because squared norm is 0" << endl);
    }
  }

  if (!constraintViolatedBefore) {
    // constraint satisfied, nothing to do
	cerr << "Rank " << rank << ", epoch " << epoch << ", constraint already satisfied" << endl;
    vector<int> status(2);
    status[0] = 0;
    status[1] = 0;
    return status;
  }

  // sanity check: constraint still violated after optimisation?
  ScoreComponentCollection newWeights(currWeights);
  newWeights.PlusEquals(weightUpdate);
  bool constraintViolatedAfter = false;
  float newDistanceFromOptimum = 0;
  featureValueDiff = featureValuesHope;
  featureValueDiff.MinusEquals(featureValuesFear);
  modelScoreDiff = featureValueDiff.InnerProduct(newWeights);
  diff = loss - (modelScoreDiff + m_margin_slack);
  // approximate comparison between floats!
  if (diff > epsilon) {
    constraintViolatedAfter = true;
    newDistanceFromOptimum += (loss - modelScoreDiff);
  }

  VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", check, constraint violated before? " << constraintViolatedBefore << ", after? " << constraintViolatedAfter << endl);
  VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", check, error before: " << oldDistanceFromOptimum << ", after: " << newDistanceFromOptimum << ", change: " << oldDistanceFromOptimum - newDistanceFromOptimum << endl);

  // apply update to weight vector
  VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", weights before update: " << currWeights << endl);
  currWeights.PlusEquals(weightUpdate);
  VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", weights after update: " << currWeights << endl);

  vector<int> status(2);
  status[0] = 1;
  status[1] = constraintViolatedAfter ? 1 : 0;
  return status;
}

}

