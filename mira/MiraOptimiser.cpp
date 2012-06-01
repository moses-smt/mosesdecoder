#include "Optimiser.h"
#include "Hildreth.h"
#include "StaticData.h"

using namespace Moses;
using namespace std;

namespace Mira {

size_t MiraOptimiser::updateWeights(
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

		  	// check if constraint is violated
		    bool violated = false;
//		    float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
		    float modelScoreDiff = oracleModelScores[i] - modelScores[i][j];
		    float diff = 0;		    

		    if (loss > modelScoreDiff) 
		    	diff = loss - modelScoreDiff;
		    cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;    
		    if (diff > epsilon) 
		    	violated = true;
		    
		    if (m_normaliseMargin) {
		      modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
		      loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
		      diff = 0;
		      if (loss > modelScoreDiff) {
			diff = loss - modelScoreDiff;
		      }
		      cerr << "Rank " << rank << ", epoch " << epoch << ", normalised constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;
		    }
		    		    
		    if (m_scale_margin) {
		      diff *= oracleBleuScores[i];
		      cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with oracle bleu score "  << oracleBleuScores[i] << endl;
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
		if (m_scale_update) {
			cerr << "Rank " << rank << ", epoch " << epoch << ", scaling summed update with oracle bleu score " << oracleBleuScores[0] << endl;
			summedUpdate.MultiplyEquals(oracleBleuScores[0]);
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
		float diff = loss - modelScoreDiff;
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
		Moses::ScoreComponentCollection& weightUpdate,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesHope,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesFear,
		const std::vector<std::vector<float> >& bleuScoresHope,
		const std::vector<std::vector<float> >& bleuScoresFear,
		const std::vector<std::vector<float> >& modelScoresHope,
		const std::vector<std::vector<float> >& modelScoresFear,
		float learning_rate,
		size_t rank,
		size_t epoch,
		int updatePosition) {

  // vector of feature values differences for all created constraints
  vector<ScoreComponentCollection> featureValueDiffs;
  vector<float> lossMinusModelScoreDiffs;
  vector<float> modelScoreDiffs;
  vector<float> all_losses;
  
  // most violated constraint in batch
  ScoreComponentCollection max_batch_featureValueDiff;
  
  // Make constraints for new hypothesis translations
  float epsilon = 0.0001;
  int violatedConstraintsBefore = 0;
  float oldDistanceFromOptimum = 0;
  
  // iterate over input sentences (1 (online) or more (batch))
  for (size_t i = 0; i < featureValuesHope.size(); ++i) {
    if (updatePosition != -1) {
      if (i < updatePosition)
	continue;
      else if (i > updatePosition)
	break;
    }
    
    // Pick all pairs[j,j] of hope and fear translations for one input sentence
    for (size_t j = 0; j < featureValuesHope[i].size(); ++j) {
      ScoreComponentCollection featureValueDiff = featureValuesHope[i][j];
      featureValueDiff.MinusEquals(featureValuesFear[i][j]);
      //cerr << "Rank " << rank << ", epoch " << epoch << ", feature value diff: " << featureValueDiff << endl;
      if (featureValueDiff.GetL1Norm() == 0) {
	cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
	continue;
      }
      
      float loss = bleuScoresHope[i][j] - bleuScoresFear[i][j];
      
      // check if constraint is violated
      bool violated = false;
      //float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
      float modelScoreDiff = modelScoresHope[i][j] - modelScoresFear[i][j];
      float diff = 0;
      if (loss > modelScoreDiff) 
	diff = loss - modelScoreDiff;
      cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;	
      
      if (diff > epsilon) 
	violated = true;
	    
      if (m_normaliseMargin) {
	modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
	loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
	diff = 0;
	if (loss > modelScoreDiff) {
	  diff = loss - modelScoreDiff;
	}
	cerr << "Rank " << rank << ", epoch " << epoch << ", normalised constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;
      }
      
      if (m_scale_margin) {
	diff *= bleuScoresHope[i][j];
	cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with oracle bleu score "  << bleuScoresHope[i][j] << endl;
      }
      
      featureValueDiffs.push_back(featureValueDiff);
      lossMinusModelScoreDiffs.push_back(diff);
      modelScoreDiffs.push_back(modelScoreDiff);
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
    if (m_scale_update) {
      cerr << "Rank " << rank << ", epoch " << epoch << ", scaling summed update with oracle bleu score " << bleuScoresHope[0][0] << endl;
      summedUpdate.MultiplyEquals(bleuScoresHope[0][0]);
    }
  }
  
  //cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << summedUpdate << endl;
  weightUpdate.PlusEquals(summedUpdate);
  
  // Sanity check: are there still violated constraints after optimisation?
/*	int violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValueDiffs.size(); ++i) {
	float modelScoreDiff = featureValueDiffs[i].InnerProduct(currWeights);
	float loss = all_losses[i];
	float diff = loss - modelScoreDiff;
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
  ScoreComponentCollection featureValueDiff = featureValuesHope;
  featureValueDiff.MinusEquals(featureValuesFear);
  if (featureValueDiff.GetL1Norm() == 0) {
    cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
    return 1;
  }

//  cerr << "Rank " << rank << ", epoch " << epoch << ", hope - fear: " << featureValueDiff << endl;
//  float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
  float modelScoreDiff = modelScoreHope - modelScoreFear;
  float loss = bleuScoreHope - bleuScoreFear;
  float diff = 0;
  if (loss > modelScoreDiff) 
    diff = loss - modelScoreDiff; 
  cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;

  if (m_normaliseMargin) {
    modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
    loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
    if (loss > modelScoreDiff) 
      diff = loss - modelScoreDiff;
    cerr << "Rank " << rank << ", epoch " << epoch << ", normalised constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;
  }
  
  if (m_scale_margin) {
	  diff *= bleuScoreHope;
	  cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with oracle bleu score "  << bleuScoreHope << endl;
  }
  if (m_scale_margin_precision) {
	  diff *= (1+m_precision);
	  cerr << "Rank " << rank << ", epoch " << epoch << ", scaling margin with 1+precision: "  << (1+m_precision) << endl;
  }

  if (diff > epsilon) {
  	// squash it between 0 and 1
  	//diff = tanh(diff);
  	//diff = (2/(1 + pow(2,-diff))) - 1;
    /*  	if (m_normaliseMargin) {
  		diff = (2/(1 + exp(-diff))) - 1;
  		cerr << "Rank " << rank << ", epoch " << epoch << ", new margin: " << diff << endl;
		}*/

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
    
    if (m_scale_update) {
  	  cerr << "Rank " << rank << ", epoch " << epoch << ", scaling update with oracle bleu score " << bleuScoreHope << endl;
  	  alpha *= bleuScoreHope;
    }
    if (m_scale_update_precision) {
  	  cerr << "Rank " << rank << ", epoch " << epoch << ", scaling update with 1+precision: " << (1+m_precision) << endl;
  	  alpha *= (1+m_precision);	
    }
    
    cerr << "Rank " << rank << ", epoch " << epoch << ", clipped/scaled alpha: " << alpha << endl;

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
  diff = loss - modelScoreDiff;
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
	float minBleuDifference = 0.005;
	for (size_t i = 0; i < featureValues.size(); ++i) {
		// Build all pairs where the first has higher Bleu than the second
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			for (size_t k = 0; k < featureValues[i].size(); ++k) {
			        if (j == k) continue;
				if (bleuScores[i][j] - minBleuDifference  < bleuScores[i][k]) // need at least a positive Bleu difference of specified amount
				  continue;
				
				ScoreComponentCollection featureValueDiff = featureValues[i][j];
				featureValueDiff.MinusEquals(featureValues[i][k]);
				//				cerr << "Rank " << rank << ", epoch " << epoch << ", feature value diff: " << featureValueDiff << endl;
				if (featureValueDiff.GetL1Norm() == 0) {
				  cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
				  continue;
				}

				cerr << "Rank " << rank << ", epoch " << epoch << ", " << bleuScores[i][j] << " - " << minBleuDifference << " >= " << bleuScores[i][k] << endl;

				float loss = bleuScores[i][j] - bleuScores[i][k];

				// check if constraint is violated
				bool violated = false;
				float modelScoreDiff = modelScores[i][j] - modelScores[i][k];
				float diff = 0;
				if (loss > modelScoreDiff) 
				  diff = loss - modelScoreDiff;
				cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff  << " >= " << loss << " (current violation: " << diff << ")" << endl;

				if (diff > epsilon) 
				  violated = true;
					
				if (m_normaliseMargin) {
				  modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
				  loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
				  diff = 0;
				  if (loss > modelScoreDiff) 
				    diff = loss - modelScoreDiff;
				  cerr << "Rank " << rank << ", epoch " << epoch << ", normalised constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;
				}
				
				featureValueDiffs.push_back(featureValueDiff);
				lossMinusModelScoreDiffs.push_back(diff);
				modelScoreDiffs.push_back(modelScoreDiff);
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
	/*int violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValueDiffs.size(); ++i) {
	        float modelScoreDiff = featureValueDiffs[i].InnerProduct(currWeights);
		float loss = all_losses[i];
		float diff = loss - modelScoreDiff;
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

size_t MiraOptimiser::updateWeightsHopeFearSelective(
		Moses::ScoreComponentCollection& weightUpdate,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesHope,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesFear,
		const std::vector<std::vector<float> >& bleuScoresHope,
		const std::vector<std::vector<float> >& bleuScoresFear,
		const std::vector<std::vector<float> >& modelScoresHope,
		const std::vector<std::vector<float> >& modelScoresFear,
		float learning_rate,
		size_t rank,
		size_t epoch,
		int updatePosition) {

  // vector of feature values differences for all created constraints
  vector<ScoreComponentCollection> nonZeroFeatures;
  vector<float> lossMinusModelScoreDiffs;
 
  // Make constraints for new hypothesis translations
  float epsilon = 0.0001;
  int violatedConstraintsBefore = 0;
  
  // iterate over input sentences (1 (online) or more (batch))
  for (size_t i = 0; i < featureValuesHope.size(); ++i) {
    if (updatePosition != -1) {
      if (i < updatePosition)
	continue;
      else if (i > updatePosition)
	break;
    }
    
    // Pick all pairs[j,j] of hope and fear translations for one input sentence
    for (size_t j = 0; j < featureValuesHope[i].size(); ++j) {
      ScoreComponentCollection featureValueDiff = featureValuesHope[i][j];
      featureValueDiff.MinusEquals(featureValuesFear[i][j]);
      if (featureValueDiff.GetL1Norm() == 0) {
	cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
	continue;
      }
           
      // check if constraint is violated
      float loss = bleuScoresHope[i][j] - bleuScoresFear[i][j];
      float modelScoreDiff = modelScoresHope[i][j] - modelScoresFear[i][j];
      float diff = 0;
      if (loss > modelScoreDiff) 
	diff = loss - modelScoreDiff;
      if (diff > epsilon) 
	++violatedConstraintsBefore;   				
      cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;	
	    
      // iterate over difference vector and add a constraint for every non-zero feature
      FVector features = featureValueDiff.GetScoresVector();
      size_t n_core = 0, n_sparse = 0, n_sparse_hope = 0, n_sparse_fear = 0;
      for (size_t i=0; i<features.coreSize(); ++i) {
	if (features[i] != 0.0) {
	  ++n_core;
	  ScoreComponentCollection f;
	  f.Assign(i, features[i]);
	  nonZeroFeatures.push_back(f);
	}
      }
      // 1 + 2 
      /*for (FVector::iterator i = features.begin(); i != features.end(); ++i) {
	if (i->second != 0.0) {
	  ++n_sparse;
	  ScoreComponentCollection f;
          f.Assign((i->first).name(), i->second);
	  nonZeroFeatures.push_back(f);
	  cerr << "Rank " << rank << ", epoch " << epoch << ", f: " << f << endl;
	}
      }
      cerr << "Rank " << rank << ", epoch " << epoch << ", non-zero features: " << nonZeroFeatures.size() << endl;*/

      // 3 
      vector<ScoreComponentCollection> nonZeroFeaturesHope;
      vector<ScoreComponentCollection> nonZeroFeaturesFear;
      for (FVector::iterator i = features.begin(); i != features.end(); ++i) {
        if (i->second != 0.0) {
          ScoreComponentCollection f;
          f.Assign((i->first).name(), i->second);
          cerr << "Rank " << rank << ", epoch " << epoch << ", f: " << f << endl;

	  if (i->second > 0.0) {
	    ++n_sparse_hope;
	    nonZeroFeaturesHope.push_back(f);
	  }
	  else {
	    ++n_sparse_fear;
	    nonZeroFeaturesFear.push_back(f);
	  }
        }
      }

      //1
      /*float n = n_core + n_sparse;
      for (size_t i=0; i<n; ++i)
	lossMinusModelScoreDiffs.push_back(diff/n);
      
      //2 
      float diff_10 = diff * 0.1;
      float diff_90 = diff * 0.9;
      cerr << "Rank " << rank << ", epoch " << epoch << ", core diff: " << diff_10/n_core << endl;
      cerr << "Rank " << rank << ", epoch " << epoch << ", sparse diff: " << diff_90/n_sparse << endl;
      for (size_t i=0; i<n_core; ++i) 
	lossMinusModelScoreDiffs.push_back(diff_10/n_core);
      for (size_t i=0; i<n_sparse; ++i) 
      lossMinusModelScoreDiffs.push_back(diff_90/n_sparse);*/

      // 3 
      float n = n_core + n_sparse_hope + n_sparse_fear;
      for (size_t i=0; i<n_core; ++i)
        lossMinusModelScoreDiffs.push_back(diff/n);      
      for (size_t i=0; i<n_sparse_hope; ++i) {
	nonZeroFeatures.push_back(nonZeroFeaturesHope[i]);
        lossMinusModelScoreDiffs.push_back((diff/n)*1.1);
      }
      for (size_t i=0; i<n_sparse_fear; ++i) {
	nonZeroFeatures.push_back(nonZeroFeaturesFear[i]);
	lossMinusModelScoreDiffs.push_back(diff/n);
      }
      cerr << "Rank " << rank << ", epoch " << epoch << ", core diff: " << diff/n << endl;
      cerr << "Rank " << rank << ", epoch " << epoch << ", hope diff: " << ((diff/n)*1.1) << endl;
      cerr << "Rank " << rank << ", epoch " << epoch << ", fear diff: " << diff/n << endl;
    }
  }

  assert(nonZeroFeatures.size() == lossMinusModelScoreDiffs.size());

  // run optimisation: compute alphas for all given constraints
  vector<float> alphas;
  ScoreComponentCollection summedUpdate;
  if (violatedConstraintsBefore > 0) {
    cerr << "Rank " << rank << ", epoch " << epoch << ", number of constraints passed to optimizer: " << nonZeroFeatures.size() << endl;
    alphas = Hildreth::optimise(nonZeroFeatures, lossMinusModelScoreDiffs, m_slack);
    
    // Update the weight vector according to the alphas and the feature value differences
    // * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
    for (size_t k = 0; k < nonZeroFeatures.size(); ++k) {
      float alpha = alphas[k];
      cerr << "Rank " << rank << ", epoch " << epoch << ", alpha: " << alpha << endl;
      if (alpha != 0) {
	ScoreComponentCollection update(nonZeroFeatures[k]);
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
    if (m_scale_update) {
      cerr << "Rank " << rank << ", epoch " << epoch << ", scaling summed update with oracle bleu score " << bleuScoresHope[0][0] << endl;
      summedUpdate.MultiplyEquals(bleuScoresHope[0][0]);
    }
  }
  
  //cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << summedUpdate << endl;
  weightUpdate.PlusEquals(summedUpdate);
  return 0;
}

size_t MiraOptimiser::updateWeightsHopeFearSummed(
		Moses::ScoreComponentCollection& weightUpdate,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesHope,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValuesFear,
		const std::vector<std::vector<float> >& bleuScoresHope,
		const std::vector<std::vector<float> >& bleuScoresFear,
		const std::vector<std::vector<float> >& modelScoresHope,
		const std::vector<std::vector<float> >& modelScoresFear,
		float learning_rate,
		size_t rank,
		size_t epoch,
		bool rescaleSlack,
		bool rewardHope,
		bool makePairs) {

  // vector of feature values differences for all created constraints
  ScoreComponentCollection averagedFeatureDiffs;
  float averagedViolations = 0;
 
  // Make constraints for new hypothesis translations
  float epsilon = 0.0001;
  int violatedConstraintsBefore = 0;
  
  if (!makePairs) {
    ScoreComponentCollection featureValueDiff;
    float lossHope = 0, lossFear = 0, modelScoreHope = 0, modelScoreFear = 0, hopeCount = 0, fearCount = 0;
    // add all hope vectors
    for (size_t i = 0; i < featureValuesHope.size(); ++i) {
      for (size_t j = 0; j < featureValuesHope[i].size(); ++j)  {
	featureValueDiff.PlusEquals(featureValuesHope[i][j]);
	lossHope += bleuScoresHope[i][j];
	modelScoreHope += modelScoresHope[i][j];
	++hopeCount;
      }
    }
    lossHope /= hopeCount;
    modelScoreHope /= hopeCount;
    
    // subtract all fear  vectors
    for (size_t i = 0; i < featureValuesFear.size(); ++i) {
      for (size_t j = 0; j < featureValuesFear[i].size(); ++j) {
        featureValueDiff.MinusEquals(featureValuesFear[i][j]);
	lossFear += bleuScoresFear[i][j];
        modelScoreFear += modelScoresFear[i][j];
	++fearCount;
      }
    }
    lossFear /= fearCount;
    modelScoreFear /= fearCount;

    if (featureValueDiff.GetL1Norm() == 0) {
      cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
      cerr << "Rank " << rank << ", epoch " << epoch << ", no constraint violated for this batch" << endl;
      return 1;
    }

    // check if constraint is violated                                                                                                       
    float lossDiff = lossHope - lossFear;
    float modelScoreDiff = modelScoreHope - modelScoreFear;
    float diff = 0;
    if (lossDiff > modelScoreDiff)
      diff = lossDiff - modelScoreDiff;
    if (diff > epsilon)
      ++violatedConstraintsBefore;
    cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " >= " << lossDiff << " (current violation: " <<\
      diff << ")" << endl;

    // add constraint                                                                                                                        
    averagedFeatureDiffs = featureValueDiff;
    averagedViolations = diff;
  }
  else {
  // iterate over input sentences (1 (online) or more (batch))
  for (size_t i = 0; i < featureValuesHope.size(); ++i) {    
    // Pick all pairs[j,j] of hope and fear translations for one input sentence and add them up 
    for (size_t j = 0; j < featureValuesHope[i].size(); ++j) {
      ScoreComponentCollection featureValueDiff = featureValuesHope[i][j];
      featureValueDiff.MinusEquals(featureValuesFear[i][j]);
      if (featureValueDiff.GetL1Norm() == 0) {
	cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
	continue;
      }
           
      // check if constraint is violated
      float lossDiff = bleuScoresHope[i][j] - bleuScoresFear[i][j];
      float modelScoreDiff = modelScoresHope[i][j] - modelScoresFear[i][j];
      if (rescaleSlack) {
	cerr << "Rank " << rank << ", epoch " << epoch << ", modelScoreDiff scaled by lossDiff: " << modelScoreDiff << " --> " << modelScoreDiff*lossDiff << endl;
	modelScoreDiff *= lossDiff; 
      }
      float diff = 0;
      if (lossDiff > modelScoreDiff) 
	diff = lossDiff - modelScoreDiff;
      if (diff > epsilon) 
	++violatedConstraintsBefore;   				
      cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " >= " << lossDiff << " (current violation: " << diff << ")" << endl;	

      // add constraint
      if (rescaleSlack) {
	averagedFeatureDiffs.MultiplyEquals(lossDiff);
	cerr << "Rank " << rank << ", epoch " << epoch << ", featureValueDiff scaled by lossDiff." << endl;
      }
      averagedFeatureDiffs.PlusEquals(featureValueDiff);
      averagedViolations += diff;
    }	    
  }
  }

  // divide by number of constraints (1/n)
  if (!makePairs) {
    averagedFeatureDiffs.DivideEquals(featureValuesHope[0].size());
  }
  else {
    averagedFeatureDiffs.DivideEquals(featureValuesHope[0].size());
    averagedViolations /= featureValuesHope[0].size();
  }
  cerr << "Rank " << rank << ", epoch " << epoch << ", averaged feature diffs: " << averagedFeatureDiffs << endl;
  cerr << "Rank " << rank << ", epoch " << epoch << ", averaged violations: " << averagedViolations << endl;

  if (violatedConstraintsBefore > 0) {
    if (rewardHope) {
      averagedFeatureDiffs.RedistributeMass(0.1);
      cerr << "Rank " << rank << ", epoch " << epoch << ", redistributed feature diffs: " << averagedFeatureDiffs << endl;
    }

    // compute alpha for given constraint: (loss diff - model score diff) / || feature value diff ||^2                                
    // featureValueDiff.GetL2Norm() * featureValueDiff.GetL2Norm() == featureValueDiff.InnerProduct(featureValueDiff)                     
    // from Crammer&Singer 2006: alpha = min {C , l_t/ ||x||^2}                                                         
    // adjusted for 1 slack according to Joachims 2009, OP4 (margin rescaling), OP5 (slack rescaling)
    float squaredNorm = averagedFeatureDiffs.GetL2Norm() * averagedFeatureDiffs.GetL2Norm();
    float alpha = averagedViolations / squaredNorm;
    cerr << "Rank " << rank << ", epoch " << epoch << ", unclipped alpha: " << alpha << endl;
    if (m_slack > 0 ) {
      if (alpha > m_slack) {
	alpha = m_slack;
      }
      else if (alpha < m_slack*(-1)) {
	alpha = m_slack*(-1);
      }
    }
    cerr << "Rank " << rank << ", epoch " << epoch << ", clipped alpha: " << alpha << endl;
  
    // compute update
    averagedFeatureDiffs.MultiplyEquals(alpha);
    weightUpdate.PlusEquals(averagedFeatureDiffs);
    return 0;
  }
  else {
    cerr << "Rank " << rank << ", epoch " << epoch << ", no constraint violated for this batch" << endl;
    return 1;
  }
}

size_t MiraOptimiser::updateWeightsRankModelSummed(
		Moses::ScoreComponentCollection& weightUpdate,
		const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValues,
		const std::vector<std::vector<float> >& bleuScores,
		const std::vector<std::vector<float> >& modelScores,
		float learning_rate,
		size_t rank,
		size_t epoch) {
  // vector of feature values differences for all created constraints                                                                                  
  ScoreComponentCollection averagedFeatureDiffs;
  float averagedViolations = 0;

  // Make constraints for new hypothesis translations
  float epsilon = 0.0001;
  int violatedConstraintsBefore = 0;
  float oldDistanceFromOptimum = 0;
  
  // iterate over input sentences (1 (online) or more (batch))
  float minBleuDifference = 0.005;
  size_t numConstraints = 0;
  for (size_t i = 0; i < featureValues.size(); ++i) {
    // Build all pairs where the first has higher Bleu than the second
    for (size_t j = 0; j < featureValues[i].size(); ++j) {
      for (size_t k = 0; k < featureValues[i].size(); ++k) {
	if (j == k) continue;
	if (bleuScores[i][j] - minBleuDifference  < bleuScores[i][k]) // need at least a positive Bleu difference of specified amount
	  continue;
	
	ScoreComponentCollection featureValueDiff = featureValues[i][j];
	featureValueDiff.MinusEquals(featureValues[i][k]);
	if (featureValueDiff.GetL1Norm() == 0) {
	  cerr << "Rank " << rank << ", epoch " << epoch << ", features equal --> skip" << endl;
	  continue;
	}
	
	// check if constraint is violated                                                                         
	float loss = bleuScores[i][j] - bleuScores[i][k];
	float modelScoreDiff = modelScores[i][j] - modelScores[i][k];
	float diff = 0;
	if (loss > modelScoreDiff)
	  diff = loss - modelScoreDiff;
	if (diff > epsilon)
	  ++violatedConstraintsBefore;
	cerr << "Rank " << rank << ", epoch " << epoch << ", constraint: " << modelScoreDiff << " >= " << loss << " (current violation: " << diff << ")" << endl;

	// add constraint                                                                                                        
	averagedFeatureDiffs.PlusEquals(featureValueDiff);
	averagedViolations += diff;
	++numConstraints;
      }
    }
  }

  // divide by number of constraints                                                                                                              
  averagedFeatureDiffs.DivideEquals(numConstraints);
  averagedViolations /= numConstraints;

  cerr << "Rank " << rank << ", epoch " << epoch << ", averaged feature diffs: " << averagedFeatureDiffs << endl;
  cerr << "Rank " << rank << ", epoch " << epoch << ", averaged violations: " << averagedViolations << endl;

  if (violatedConstraintsBefore > 0) {
    // compute alpha for given constraint: (loss - model score diff) / || feature value diff ||^2                                                          
    // featureValueDiff.GetL2Norm() * featureValueDiff.GetL2Norm() == featureValueDiff.InnerProduct(featureValueDiff)                                
    // from Crammer&Singer 2006: alpha = min {C , l_t/ ||x||^2}                                                                                         
    float squaredNorm = averagedFeatureDiffs.GetL2Norm() * averagedFeatureDiffs.GetL2Norm();
    float alpha = averagedViolations / squaredNorm;
    cerr << "Rank " << rank << ", epoch " << epoch << ", unclipped alpha: " << alpha << endl;
    if (m_slack > 0 ) {
      if (alpha > m_slack) {
        alpha = m_slack;
      }
      else if (alpha < m_slack*(-1)) {
        alpha = m_slack*(-1);
      }
    }
    cerr << "Rank " << rank << ", epoch " << epoch << ", clipped alpha: " << alpha << endl;

    // compute update                                                                                                                            
    averagedFeatureDiffs.MultiplyEquals(alpha);
    weightUpdate.PlusEquals(averagedFeatureDiffs);
    return 0;
  }
  else {
    cerr << "Rank " << rank << ", epoch " << epoch << ", no constraint violated for this batch" << endl;
    return 1;
  }
}

}

