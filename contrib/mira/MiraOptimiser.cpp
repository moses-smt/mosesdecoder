#include <algorithm>
#include "Optimiser.h"
#include "Hildreth.h"
#include "moses/StaticData.h"

using namespace Moses;
using namespace std;

namespace Mira
{

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
  size_t epoch)
{

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
  } else {
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
  int updatePosition)
{

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
      if (int(i) < updatePosition)
        continue;
      else if (int(i) > updatePosition)
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
          float factor = std::min(1.5f, (float) log2(bleuScoresHope[0][0])); // TODO: make independent of number of oracles!!
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
  } else {
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
  size_t epoch)
{

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
      } else if (alpha < m_slack*(-1)) {
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

    cerr << "Rank " << rank << ", epoch " << epoch << ", clipped/scaled alpha: " << alpha << endl;

    // apply boosting factor
    if (m_boost && modelScoreDiff <= 0) {
      // factor between 1.5 and 3 (for Bleu scores between 5 and 20, the factor is within the boundaries)
      float factor = min(1.5f, (float) log2(bleuScoreHope));
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

}

