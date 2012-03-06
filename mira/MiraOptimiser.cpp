#include "Optimiser.h"
#include "Hildreth.h"
#include "StaticData.h"

using namespace Moses;
using namespace std;

namespace Mira {

size_t MiraOptimiser::updateWeights(
		ScoreComponentCollection& currWeights,
		ScoreComponentCollection& weightUpdate,
    const vector<vector<ScoreComponentCollection> >& featureValues,
    const vector<vector<float> >& losses,
    const vector<vector<float> >& bleuScores,
    const vector<vector<float> >& modelScores,
    const vector<ScoreComponentCollection>& oracleFeatureValues,
    const vector<float> oracleBleuScores,
    const vector<float> oracleModelScores,
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

			//			cerr << "Rank " << rank << ", epoch " << epoch << ", feature value diff: " << featureValueDiff << endl;
			if (featureValueDiff.GetL1Norm() == 0) {
				cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
				continue;
			}

			float loss = losses[i][j];
		    if (m_scale_margin == 1) {
		    	loss *= oracleBleuScores[i];
		    	cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with oracle bleu score "  << oracleBleuScores[i] << endl;
		    }
		    else if (m_scale_margin == 2) {
		    	loss *= log2(oracleBleuScores[i]);
		    	cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with log2 oracle bleu score "  << log2(oracleBleuScores[i]) << endl;
		    }
		    else if (m_scale_margin == 10) {
		    	loss *= log10(oracleBleuScores[i]);
		    	cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with log10 oracle bleu score "  << log10(oracleBleuScores[i]) << endl;
		    }

		  	// check if constraint is violated
		    bool violated = false;
		    bool addConstraint = true;
//		    float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
		    float modelScoreDiff = oracleModelScores[i] - modelScores[i][j];
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

		    float lossMinusModelScoreDiff = loss - (modelScoreDiff + m_margin_slack);
		    if (addConstraint) {
		    	if (m_normaliseMargin)
		    		lossMinusModelScoreDiff = (2/(1 + exp(- lossMinusModelScoreDiff))) - 1;

		    	featureValueDiffs.push_back(featureValueDiff);
		    	lossMinusModelScoreDiffs.push_back(lossMinusModelScoreDiff);
		    	all_losses.push_back(loss);

		    	if (violated) {
		    		if (m_normaliseMargin)
		    			diff = (2/(1 + exp(- diff))) - 1;

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
	  	cerr << "Rank " << rank << ", epoch " << epoch << ", alpha: " << alpha << endl;
	  	ScoreComponentCollection update(featureValueDiffs[k]);
	    update.MultiplyEquals(alpha);
	    
	    // sum updates
	    summedUpdate.PlusEquals(update);
	  }
	} 
	else {
		cerr << "Rank " << rank << ", epoch " << epoch << ", no constraint violated for this batch" << endl;
//		return 0;
		return 1;
	}

	// apply learning rate
	if (learning_rate != 1) {
		cerr << "Rank " << rank << ", epoch " << epoch << ", apply learning rate " << learning_rate << " to update." << endl;
		summedUpdate.MultiplyEquals(learning_rate);
	}

	// scale update by BLEU of oracle (for batch size 1 only)
	if (oracleBleuScores.size() == 1) {
		if (m_scale_update == 1) {
			cerr << "Rank " << rank << ", epoch " << epoch << ", scaling summed update with oracle bleu score " << oracleBleuScores[0] << endl;
			summedUpdate.MultiplyEquals(oracleBleuScores[0]);
		}
		else if (m_scale_update == 2) {
			cerr << "Rank " << rank << ", epoch " << epoch << ", scaling summed update with log2 oracle bleu score " << log2(oracleBleuScores[0]) << endl;
			summedUpdate.MultiplyEquals(log2(oracleBleuScores[0]));
		}
		else if (m_scale_update == 10) {
			cerr << "Rank " << rank << ", epoch " << epoch << ", scaling summed update with log10 oracle bleu score " << log10(oracleBleuScores[0]) << endl;
			summedUpdate.MultiplyEquals(log10(oracleBleuScores[0]));
		}
	}

	//	cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << summedUpdate << endl;
	weightUpdate.PlusEquals(summedUpdate);

	// Sanity check: are there still violated constraints after optimisation?
/*	int violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValueDiffs.size(); ++i) {
		float modelScoreDiff = featureValueDiffs[i].InnerProduct(currWeights);
		float loss = all_losses[i];
		float diff = loss - (modelScoreDiff + m_margin_slack);
		if (diff > epsilon) {
			++violatedConstraintsAfter;
			newDistanceFromOptimum += diff;
		}
	}
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", violated constraint before: " << violatedConstraintsBefore << ", after: " << violatedConstraintsAfter  << ", change: " << violatedConstraintsBefore - violatedConstraintsAfter << endl);
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", error before: " << oldDistanceFromOptimum << ", after: " << newDistanceFromOptimum << ", change: " << oldDistanceFromOptimum - newDistanceFromOptimum << endl);*/
//	return violatedConstraintsAfter;
	return 0;
}

size_t MiraOptimiser::updateWeightsHopeFear(
		Moses::ScoreComponentCollection& currWeights,
		Moses::ScoreComponentCollection& weightUpdate,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesHope,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesFear,
		const std::vector<std::vector<float> >& bleuScoresHope,
		const std::vector<std::vector<float> >& bleuScoresFear,
		const std::vector<std::vector<float> >& modelScoresHope,
		const std::vector<std::vector<float> >& modelScoresFear,
		float learning_rate,
		size_t rank,
		size_t epoch) {

	// vector of feature values differences for all created constraints
	vector<ScoreComponentCollection> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> modelScoreDiffs;
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
		// Pair all hope translations with all fear translations for one input sentence
		for (size_t j = 0; j < featureValuesHope[i].size(); ++j) {
			for (size_t k = 0; k < featureValuesFear[i].size(); ++k) {
				ScoreComponentCollection featureValueDiff = featureValuesHope[i][j];
				featureValueDiff.MinusEquals(featureValuesFear[i][k]);
				//				cerr << "Rank " << rank << ", epoch " << epoch << ", feature value diff: " << featureValueDiff << endl;
				if (featureValueDiff.GetL1Norm() == 0) {
					cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
					continue;
				}

				float loss = bleuScoresHope[i][j] - bleuScoresFear[i][k];
				if (m_scale_margin == 1) {
					loss *= bleuScoresHope[i][j];
					cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with oracle bleu score "  << bleuScoresHope[i][j] << endl;
				}
				else if (m_scale_margin == 2) {
					loss *= log2(bleuScoresHope[i][j]);
					cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with log2 oracle bleu score "  << log2(bleuScoresHope[i][j]) << endl;
				}
				else if (m_scale_margin == 10) {
					loss *= log10(bleuScoresHope[i][j]);
					cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with log10 oracle bleu score "  << log10(bleuScoresHope[i][j]) << endl;
				}

				// check if constraint is violated
				bool violated = false;
				bool addConstraint = true;
//				float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
				float modelScoreDiff = modelScoresHope[i][j] - modelScoresFear[i][k];
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

				float lossMinusModelScoreDiff = loss - (modelScoreDiff + m_margin_slack);
				if (addConstraint) {
					if (m_normaliseMargin)
						lossMinusModelScoreDiff = (2/(1 + exp(- lossMinusModelScoreDiff))) - 1;

					featureValueDiffs.push_back(featureValueDiff);
					lossMinusModelScoreDiffs.push_back(lossMinusModelScoreDiff);
					modelScoreDiffs.push_back(modelScoreDiff);
					all_losses.push_back(loss);

					if (violated) {
						if (m_normaliseMargin)
							diff = (2/(1 + exp(- diff))) - 1;

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
	  	cerr << "Rank " << rank << ", epoch " << epoch << ", alpha: " << alpha << endl;
	  	if (alpha != 0) {
	  		// apply boosting factor
	  		if (m_boost && modelScoreDiffs[k] <= 0) {
	  			// factor between 1.5 and 3 (for Bleu scores between 5 and 20, the factor is within the boundaries)
	  			float factor = min(1.5, log2(bleuScoresHope[0][0])); // TODO: make independent of number of oracles!!
	  			factor = min(3.0f, factor);
	  			alpha = alpha * factor;
	  			cerr << "Rank " << rank << ", epoch " << epoch << ", apply boosting factor " << factor << " to update." << endl;
	  		}

	  		ScoreComponentCollection update(featureValueDiffs[k]);
	  		update.MultiplyEquals(alpha);

	  		// sum updates
	  		summedUpdate.PlusEquals(update);
	  	}
	  }
	}
	else {
		cerr << "Rank " << rank << ", epoch " << epoch << ", no constraint violated for this batch" << endl;
//	  return 0;
		return 1;
	}

	// apply learning rate
	if (learning_rate != 1) {
		cerr << "Rank " << rank << ", epoch " << epoch << ", apply learning rate " << learning_rate << " to update." << endl;
		summedUpdate.MultiplyEquals(learning_rate);
	}

	// scale update by BLEU of oracle (for batch size 1 only)
	if (featureValuesHope.size() == 1) {
		if (m_scale_update == 1) {
			cerr << "Rank " << rank << ", epoch " << epoch << ", scaling summed update with oracle bleu score " << bleuScoresHope[0][0] << endl;
			summedUpdate.MultiplyEquals(bleuScoresHope[0][0]);
		}
		else if (m_scale_update == 2) {
			cerr << "Rank " << rank << ", epoch " << epoch << ", scaling summed update with log2 oracle bleu score " << log2(bleuScoresHope[0][0]) << endl;
			summedUpdate.MultiplyEquals(log2(bleuScoresHope[0][0]));
		}
		else if (m_scale_update == 10) {
			cerr << "Rank " << rank << ", epoch " << epoch << ", scaling summed update with log10 oracle bleu score " << log10(bleuScoresHope[0][0]) << endl;
			summedUpdate.MultiplyEquals(log10(bleuScoresHope[0][0]));
		}
	}

	//	cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << summedUpdate << endl;
	weightUpdate.PlusEquals(summedUpdate);

	// Sanity check: are there still violated constraints after optimisation?
/*	int violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValueDiffs.size(); ++i) {
		float modelScoreDiff = featureValueDiffs[i].InnerProduct(currWeights);
		float loss = all_losses[i];
		float diff = loss - (modelScoreDiff + m_margin_slack);
		if (diff > epsilon) {
			++violatedConstraintsAfter;
			newDistanceFromOptimum += diff;
		}
	}
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", violated constraint before: " << violatedConstraintsBefore << ", after: " << violatedConstraintsAfter  << ", change: " << violatedConstraintsBefore - violatedConstraintsAfter << endl);
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", error before: " << oldDistanceFromOptimum << ", after: " << newDistanceFromOptimum << ", change: " << oldDistanceFromOptimum - newDistanceFromOptimum << endl);*/
//	return violatedConstraintsAfter;
	return 0;
}

size_t MiraOptimiser::updateWeightsAnalytically(
		ScoreComponentCollection& currWeights,
		ScoreComponentCollection& weightUpdate,
		ScoreComponentCollection& featureValuesHope,
    ScoreComponentCollection& featureValuesFear,
    float bleuScoreHope,
    float bleuScoreFear,
    float modelScoreHope,
    float modelScoreFear,
    float learning_rate,
    size_t rank,
    size_t epoch) {

  float epsilon = 0.0001;
  float oldDistanceFromOptimum = 0;
  bool constraintViolatedBefore = false;

 // cerr << "Rank " << rank << ", epoch " << epoch << ", hope: " << featureValuesHope << endl;
 // cerr << "Rank " << rank << ", epoch " << epoch << ", fear: " << featureValuesFear << endl;

  // scenario 1: reward only-hope, penalize only-fear
  // scenario 2: reward all-hope, penalize only-fear
  // scenario 3: reward all-hope
  // scenario 4: reward strongly only-hope, reward mildly all-hope
  // scenario 5: reward strongly only-hope, reward mildly all-hope, penalize only-fear
  // scenario 6: reward only-hope
  // scenario 7: penalize only-fear

  ScoreComponentCollection featureValueDiff;
  switch (m_update_scheme) {
  case 2:
	  // values: 1: all-hope, -1: only-fear
	  featureValueDiff = featureValuesHope;
	  featureValueDiff.MinusEquals(featureValuesFear);
	  featureValueDiff.SparsePlusEquals(featureValuesHope);
	  //max: 1 (set all 2 to 1)
	  featureValueDiff.CapMax(1);
	  break;
  case 3:
	  // values: 1: all-hope
	  featureValueDiff = featureValuesHope;
	  break;
  case 4:
	  // values: 2: only-hope, 1: both
	  featureValueDiff = featureValuesHope;
	  featureValueDiff.MinusEquals(featureValuesFear);
	  featureValueDiff.SparsePlusEquals(featureValuesHope);
	  // min: 0 (set all -1 to 0)
	  featureValueDiff.CapMin(0);
	  break;
  case 5:
	  // values: 2: only-hope, 1: both, -1: only-fear
	  featureValueDiff = featureValuesHope;
	  featureValueDiff.MinusEquals(featureValuesFear);
	  featureValueDiff.SparsePlusEquals(featureValuesHope);
	  break;
  case 6:
  	// values: 1: only-hope
  	featureValueDiff = featureValuesHope;
  	featureValueDiff.MinusEquals(featureValuesFear);
  	// min: 0 (set all -1 to 0)
  	featureValueDiff.CapMin(0);
  	break;
  case 7:
  	// values: -1: only-fear
  	featureValueDiff = featureValuesHope;
  	featureValueDiff.MinusEquals(featureValuesFear);
  	// max: 0 (set all 1 to 0)
  	featureValueDiff.CapMax(0);
  	break;
  case 1:
  default:
	  // values: 1: only-hope, -1: only-fear
	  featureValueDiff = featureValuesHope;
	  featureValueDiff.MinusEquals(featureValuesFear);
	  break;
  }

	if (featureValueDiff.GetL1Norm() == 0) {
		cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
		return 1;
	}

//  cerr << "Rank " << rank << ", epoch " << epoch << ", hope - fear: " << featureValueDiff << endl;
//  float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
  float modelScoreDiff = modelScoreHope - modelScoreFear;
  float loss = bleuScoreHope - bleuScoreFear;
  float diff = 0;
  if (loss > (modelScoreDiff + m_margin_slack)) {
  	diff = loss - (modelScoreDiff + m_margin_slack);
  }
  cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " + " << m_margin_slack << " >= " << loss << " (current violation: " << diff << ")" << endl;

  if (diff > epsilon) {
  	// squash it between 0 and 1
  	//diff = tanh(diff);
  	//diff = (2/(1 + pow(2,- diff))) - 1;
  	if (m_normaliseMargin)
  		diff = (2/(1 + exp(- diff))) - 1;

    // constraint violated
    oldDistanceFromOptimum += diff;
    constraintViolatedBefore = true;

    // compute alpha for given constraint: (loss - model score diff) / || feature value diff ||^2
    // featureValueDiff.GetL2Norm() * featureValueDiff.GetL2Norm() == featureValueDiff.InnerProduct(featureValueDiff)
    // from Crammer&Singer 2006: alpha = min {C , l_t/ ||x||^2}
    float squaredNorm = featureValueDiff.GetL2Norm() * featureValueDiff.GetL2Norm();

    float alpha = diff / squaredNorm;
    cerr << "Rank " << rank << ", epoch " << epoch << ", unclipped alpha: " << alpha << endl;
    if (m_slack > 0 ) {
    	if (alpha > m_slack) {
    		alpha = m_slack;
    	}
    	else if (alpha < m_slack*(-1)) {
    		alpha = m_slack*(-1);
    	}
    }

    // apply learning rate
    if (learning_rate != 1)
    	alpha = alpha * learning_rate;
    cerr << "Rank " << rank << ", epoch " << epoch << ", clipped alpha: " << alpha << endl;

    // apply boosting factor
    if (m_boost && modelScoreDiff <= 0) {
    	// factor between 1.5 and 3 (for Bleu scores between 5 and 20, the factor is within the boundaries)
    	float factor = min(1.5, log2(bleuScoreHope));
    	factor = min(3.0f, factor);
    	alpha = alpha * factor;
    	cerr << "Rank " << rank << ", epoch " << epoch << ", boosted alpha: " << alpha << endl;
    }

    featureValueDiff.MultiplyEquals(alpha);
    weightUpdate.PlusEquals(featureValueDiff);
//  	cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << weightUpdate << endl;
  }

  if (!constraintViolatedBefore) {
    // constraint satisfied, nothing to do
  	cerr << "Rank " << rank << ", epoch " << epoch << ", constraint already satisfied" << endl;
    return 1;
  }

  // sanity check: constraint still violated after optimisation?
/*  ScoreComponentCollection newWeights(currWeights);
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

  float hopeScore = featureValuesHope.InnerProduct(newWeights);
  float fearScore = featureValuesFear.InnerProduct(newWeights);
  cerr << "New hope score: " << hopeScore << endl;
  cerr << "New fear score: " << fearScore << endl;

  VERBOSE(0, "Rank " << rank << ", epoch " << epoch << ", check, constraint violated before? " << constraintViolatedBefore << ", after? " << constraintViolatedAfter << endl);
  VERBOSE(0, "Rank " << rank << ", epoch " << epoch << ", check, error before: " << oldDistanceFromOptimum << ", after: " << newDistanceFromOptimum << ", change: " << oldDistanceFromOptimum - newDistanceFromOptimum << endl);
*/
  return 0;
}

size_t MiraOptimiser::updateWeightsRankModel(
		Moses::ScoreComponentCollection& currWeights,
		Moses::ScoreComponentCollection& weightUpdate,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValues,
		const std::vector<std::vector<float> >& bleuScores,
		const std::vector<std::vector<float> >& modelScores,
		float learning_rate,
		size_t rank,
		size_t epoch) {

	// vector of feature values differences for all created constraints
	vector<ScoreComponentCollection> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> modelScoreDiffs;
	vector<float> all_losses;

	// Make constraints for new hypothesis translations
	float epsilon = 0.0001;
	int violatedConstraintsBefore = 0;
	float oldDistanceFromOptimum = 0;

	// iterate over input sentences (1 (online) or more (batch))
	for (size_t i = 0; i < featureValues.size(); ++i) {
		// Build all pairs where the first has higher Bleu than the second
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			for (size_t k = 0; k < featureValues[i].size(); ++k) {
				if (bleuScores[i][j] <= bleuScores[i][k]) // skip examples where the first has lower Bleu than the second
					continue;

				ScoreComponentCollection featureValueDiff = featureValues[i][j];
				featureValueDiff.MinusEquals(featureValues[i][k]);
				//				cerr << "Rank " << rank << ", epoch " << epoch << ", feature value diff: " << featureValueDiff << endl;
				if (featureValueDiff.GetL1Norm() == 0)
					cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
					continue;

				float loss = bleuScores[i][j] - bleuScores[i][k];

				// check if constraint is violated
				bool violated = false;
				bool addConstraint = true;
				float modelScoreDiff = modelScores[i][j] - modelScores[i][k];
				float diff = 0;
				if (loss > (modelScoreDiff)) {
					diff = loss - (modelScoreDiff);
				}
				cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff  << " >= " << loss << " (current violation: " << diff << ")" << endl;

				if (diff > epsilon) {
					violated = true;
				}
				else if (m_onlyViolatedConstraints) {
					addConstraint = false;
				}

				float lossMinusModelScoreDiff = loss - modelScoreDiff;
				if (addConstraint) {
					if (m_normaliseMargin)
						lossMinusModelScoreDiff = (2/(1 + exp(- lossMinusModelScoreDiff))) - 1;

					featureValueDiffs.push_back(featureValueDiff);
					lossMinusModelScoreDiffs.push_back(lossMinusModelScoreDiff);
					modelScoreDiffs.push_back(modelScoreDiff);
					all_losses.push_back(loss);

					if (violated) {
						if (m_normaliseMargin)
							diff = (2/(1 + exp(- diff))) - 1;

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
	  	cerr << "Rank " << rank << ", epoch " << epoch << ", alpha: " << alpha << endl;
	  	if (alpha != 0) {
	  		// apply boosting factor
	  		if (m_boost && modelScoreDiffs[k] <= 0) {
	  			// factor between 1.5 and 3 (for Bleu scores between 5 and 20, the factor is within the boundaries)
	  			float factor = min(1.5, log2(bleuScores[0][0])); // TODO: make independent of number of oracles!!
	  			factor = min(3.0f, factor);
	  			alpha = alpha * factor;
	  			cerr << "Rank " << rank << ", epoch " << epoch << ", apply boosting factor " << factor << " to update." << endl;
	  		}

	  		ScoreComponentCollection update(featureValueDiffs[k]);
	  		update.MultiplyEquals(alpha);

	  		// sum updates
	  		summedUpdate.PlusEquals(update);
	  	}
	  }
	}
	else {
		cerr << "Rank " << rank << ", epoch " << epoch << ", no constraint violated for this batch" << endl;
//	  return 0;
		return 1;
	}

	// apply learning rate
	if (learning_rate != 1) {
		cerr << "Rank " << rank << ", epoch " << epoch << ", apply learning rate " << learning_rate << " to update." << endl;
		summedUpdate.MultiplyEquals(learning_rate);
	}

	//	cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << summedUpdate << endl;
	weightUpdate.PlusEquals(summedUpdate);

	// Sanity check: are there still violated constraints after optimisation?
	int violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValueDiffs.size(); ++i) {
		float modelScoreDiff = featureValueDiffs[i].InnerProduct(currWeights);
		float loss = all_losses[i];
		float diff = loss - (modelScoreDiff + m_margin_slack);
		if (diff > epsilon) {
			++violatedConstraintsAfter;
			newDistanceFromOptimum += diff;
		}
	}
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", violated constraint before: " << violatedConstraintsBefore << ", after: " << violatedConstraintsAfter  << ", change: " << violatedConstraintsBefore - violatedConstraintsAfter << endl);
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", error before: " << oldDistanceFromOptimum << ", after: " << newDistanceFromOptimum << ", change: " << oldDistanceFromOptimum - newDistanceFromOptimum << endl);
//	return violatedConstraintsAfter;
	return 0;
}

size_t MiraOptimiser::updateWeightsHopeFearAndRankModel(
		Moses::ScoreComponentCollection& currWeights,
		Moses::ScoreComponentCollection& weightUpdate,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesHope,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesFear,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValues,
		const std::vector<std::vector<float> >& bleuScoresHope,
		const std::vector<std::vector<float> >& bleuScoresFear,
		const std::vector<std::vector<float> >& bleuScores,
		const std::vector<std::vector<float> >& modelScoresHope,
		const std::vector<std::vector<float> >& modelScoresFear,
		const std::vector<std::vector<float> >& modelScores,
		float learning_rate,
		size_t rank,
		size_t epoch) {

	// vector of feature values differences for all created constraints
	vector<ScoreComponentCollection> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> modelScoreDiffs;
	vector<float> all_losses;

	// Make constraints for new hypothesis translations
	float epsilon = 0.0001;
	int violatedConstraintsBefore = 0;
	float oldDistanceFromOptimum = 0;

	// HOPE-FEAR: iterate over input sentences (1 (online) or more (batch))
	for (size_t i = 0; i < featureValuesHope.size(); ++i) {
		// Pair all hope translations with all fear translations for one input sentence
		for (size_t j = 0; j < featureValuesHope[i].size(); ++j) {
			for (size_t k = 0; k < featureValuesFear[i].size(); ++k) {
				ScoreComponentCollection featureValueDiff = featureValuesHope[i][j];
				featureValueDiff.MinusEquals(featureValuesFear[i][k]);
				//				cerr << "Rank " << rank << ", epoch " << epoch << ", feature value diff: " << featureValueDiff << endl;
				if (featureValueDiff.GetL1Norm() == 0) {
					cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
					continue;
				}

				float loss = bleuScoresHope[i][j] - bleuScoresFear[i][k];

				// check if constraint is violated
				bool violated = false;
				bool addConstraint = true;
//				float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
				float modelScoreDiff = modelScoresHope[i][j] - modelScoresFear[i][k];
				float diff = 0;
				if (loss > modelScoreDiff) {
					diff = loss - modelScoreDiff;
				}
				cerr << "Rank " << rank << ", epoch " << epoch << ", hope-fear constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;

				if (diff > epsilon) {
					violated = true;
				}
				else if (m_onlyViolatedConstraints) {
					addConstraint = false;
				}

				float lossMinusModelScoreDiff = loss - modelScoreDiff;
				if (addConstraint) {
					if (m_normaliseMargin)
						lossMinusModelScoreDiff = (2/(1 + exp(- lossMinusModelScoreDiff))) - 1;

					featureValueDiffs.push_back(featureValueDiff);
					lossMinusModelScoreDiffs.push_back(lossMinusModelScoreDiff);
					modelScoreDiffs.push_back(modelScoreDiff);
					all_losses.push_back(loss);

					if (violated) {
						if (m_normaliseMargin)
							diff = (2/(1 + exp(- diff))) - 1;

						++violatedConstraintsBefore;
						oldDistanceFromOptimum += diff;
					}
				}
			}
		}
	}

	// MODEL: iterate over input sentences (1 (online) or more (batch))
	for (size_t i = 0; i < featureValues.size(); ++i) {
		// Build all pairs where the first has higher Bleu than the second
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			for (size_t k = 0; k < featureValues[i].size(); ++k) {
				if (bleuScores[i][j] <= bleuScores[i][k]) // skip examples where the first has lower Bleu than the second
					continue;

				ScoreComponentCollection featureValueDiff = featureValues[i][j];
				featureValueDiff.MinusEquals(featureValues[i][k]);
				//				cerr << "Rank " << rank << ", epoch " << epoch << ", feature value diff: " << featureValueDiff << endl;
				if (featureValueDiff.GetL1Norm() == 0)
					// skip constraint
					continue;

				float loss = bleuScores[i][j] - bleuScores[i][k];

				// check if constraint is violated
				bool violated = false;
				bool addConstraint = true;
				float modelScoreDiff = modelScores[i][j] - modelScores[i][k];
				float diff = 0;
				if (loss > (modelScoreDiff)) {
					diff = loss - (modelScoreDiff);
				}
				cerr << "Rank " << rank << ", epoch " << epoch << ", model constraint: " << modelScoreDiff  << " >= " << loss << " (current violation: " << diff << ")" << endl;

				if (diff > epsilon) {
					violated = true;
				}
				else if (m_onlyViolatedConstraints) {
					addConstraint = false;
				}

				float lossMinusModelScoreDiff = loss - modelScoreDiff;
				if (addConstraint) {
					if (m_normaliseMargin)
						lossMinusModelScoreDiff = (2/(1 + exp(- lossMinusModelScoreDiff))) - 1;

					featureValueDiffs.push_back(featureValueDiff);
					lossMinusModelScoreDiffs.push_back(lossMinusModelScoreDiff);
					modelScoreDiffs.push_back(modelScoreDiff);
					all_losses.push_back(loss);

					if (violated) {
						if (m_normaliseMargin)
							diff = (2/(1 + exp(- diff))) - 1;

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
	  	cerr << "Rank " << rank << ", epoch " << epoch << ", alpha: " << alpha << endl;
	  	if (alpha != 0) {
	  		// apply boosting factor
	  		if (m_boost && modelScoreDiffs[k] <= 0) {
	  			// factor between 1.5 and 3 (for Bleu scores between 5 and 20, the factor is within the boundaries)
	  			float factor = min(1.5, log2(bleuScores[0][0])); // TODO: make independent of number of oracles!!
	  			factor = min(3.0f, factor);
	  			alpha = alpha * factor;
	  			cerr << "Rank " << rank << ", epoch " << epoch << ", apply boosting factor " << factor << " to update." << endl;
	  		}

	  		ScoreComponentCollection update(featureValueDiffs[k]);
	  		update.MultiplyEquals(alpha);

	  		// sum updates
	  		summedUpdate.PlusEquals(update);
	  	}
	  }
	}
	else {
		cerr << "Rank " << rank << ", epoch " << epoch << ", no constraint violated for this batch" << endl;
//	  return 0;
		return 1;
	}

	// apply learning rate
	if (learning_rate != 1) {
		cerr << "Rank " << rank << ", epoch " << epoch << ", apply learning rate " << learning_rate << " to update." << endl;
		summedUpdate.MultiplyEquals(learning_rate);
	}

	//	cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << summedUpdate << endl;
	weightUpdate.PlusEquals(summedUpdate);

	// Sanity check: are there still violated constraints after optimisation?
	int violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValueDiffs.size(); ++i) {
		float modelScoreDiff = featureValueDiffs[i].InnerProduct(currWeights);
		float loss = all_losses[i];
		float diff = loss - (modelScoreDiff + m_margin_slack);
		if (diff > epsilon) {
			++violatedConstraintsAfter;
			newDistanceFromOptimum += diff;
		}
	}
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", violated constraint before: " << violatedConstraintsBefore << ", after: " << violatedConstraintsAfter  << ", change: " << violatedConstraintsBefore - violatedConstraintsAfter << endl);
	VERBOSE(1, "Rank " << rank << ", epoch " << epoch << ", error before: " << oldDistanceFromOptimum << ", after: " << newDistanceFromOptimum << ", change: " << oldDistanceFromOptimum - newDistanceFromOptimum << endl);
//	return violatedConstraintsAfter;
	return 0;
}

}

