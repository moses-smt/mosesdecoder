#include "Optimiser.h"
#include "Hildreth.h"
#include "../StaticData.h"

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

			if (featureValueDiff.GetL1Norm() == 0) { // over sparse & core features values
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
//		    cerr<<"Loss ("<<loss<<") - modelScoreDiff ("<<modelScoreDiff<<") = "<<diff<<"\n";
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
//		cerr<<"Features values diff size : "<<featureValueDiffs.size() << " (of which violated: " << violatedConstraintsBefore << ")" << endl;
	  if (m_slack != 0) {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
	    cerr<<"Alphas : ";
	    for (int i=0;i<alphas.size();i++)
	    {
	    	cerr<<alphas[i]<<" ";
	    }
	    cerr<<"\n";
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
	weightUpdate.PlusEquals(summedUpdate);

	return 0;
}

size_t MiraOptimiser::updateSparseWeights(
	SparseVec& UpdateVector,			// sparse vector
	const vector<vector<int> >& FeatureValues,	// index to hypothesis feature values in UpdateVector
    const vector<float>& losses,
    const vector<float>& bleuScores,
    const vector<float>& modelScores,
    const vector<vector<int> >& oracleFeatureValues,	// index to oracle feature values
    const float oracleBleuScores,
    const float oracleModelScores,
    float learning_rate) {

	vector<SparseVec> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> all_losses;
	// Make constraints for new hypothesis translations
	float epsilon = 0.0001;
	int violatedConstraintsBefore = 0;
	SparseVec featureValueDiff(UpdateVector.GetSize());

	// loop iterating over all oracles.
	for(int j=0;j<FeatureValues.size();j++){

		for(int i=0;i<oracleFeatureValues[0].size();i++){
			featureValueDiff.Assign(oracleFeatureValues[0][i],UpdateVector.getElement(oracleFeatureValues[0][i]));
//			cerr<<"Inserting from Oracle at : "<<oracleFeatureValues[0][i]<<" value : "<<UpdateVector.getElement(oracleFeatureValues[0][i])<<endl;
		}

		for(int i=0;i<FeatureValues[j].size();i++){
			featureValueDiff.MinusEqualsFeat(FeatureValues[j][i],UpdateVector.getElement(FeatureValues[j][i]));
//			cerr<<"Subtracting from features at : "<<oracleFeatureValues[0][i]<<" value : "<<UpdateVector.getElement(oracleFeatureValues[0][i])<<endl;
		}

		if (featureValueDiff.GetL1Norm() == 0) { // over sparse features values only
			continue;
		}

		float loss=losses[j];
//		cerr<<"Loss : "<<loss<<endl;
		bool violated = false;
		float modelScoreDiff = oracleModelScores - modelScores[j];
		float diff = 0;
		if (loss > modelScoreDiff)
			diff = loss - modelScoreDiff;
//		cerr<<"Diff : "<<diff<<endl;
		if (diff > epsilon){
			violated = true;
//			cerr<<"Constraint violated!!!\n";
		}

		if (m_normaliseMargin) {
			modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
			loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
			diff = 0;
			if (loss > modelScoreDiff) {
				diff = loss - modelScoreDiff;
			}
		}
		if (m_scale_margin) {
			diff *= oracleBleuScores;
		}
		featureValueDiffs.push_back(featureValueDiff);
		lossMinusModelScoreDiffs.push_back(diff);
		all_losses.push_back(loss);
		if (violated) {
			++violatedConstraintsBefore;
		}
	}
	vector<float> alphas;
	SparseVec summedUpdate(UpdateVector.GetSize());
	if (violatedConstraintsBefore > 0) {
		if (m_slack != 0) {
			alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
			cerr<<"Alphas : ";for(int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<endl;
		} else {
			alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs);
		}
		for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
			float alpha = alphas[k];
			SparseVec update(featureValueDiffs[k]);
			update.MultiplyEquals(alpha);
			// sum updates
			summedUpdate.PlusEquals(update);
		}
	}
	else {
		return 1;
	}
	if (learning_rate != 1) {
		summedUpdate.MultiplyEquals(learning_rate);
	}

	if (m_scale_update) {
		summedUpdate.MultiplyEquals(oracleBleuScores);
	}
	UpdateVector.PlusEquals(summedUpdate);

	return 0;
}



}

