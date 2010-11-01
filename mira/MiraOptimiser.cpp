#include "Optimiser.h"
#include "Hildreth.h"

using namespace Moses;
using namespace std;

namespace Mira {

void MiraOptimiser::updateWeights(ScoreComponentCollection& currWeights,
		const vector< vector<ScoreComponentCollection> >& featureValues,
		const vector< vector<float> >& losses,
		const ScoreComponentCollection& oracleFeatureValues) {

	if (m_hildreth) {
		size_t numberOfViolatedConstraints = 0;
		vector< ScoreComponentCollection> featureValueDiffs;
		vector< float> lossMarginDistances;
		for (size_t i = 0; i < featureValues.size(); ++i) {
			for (size_t j = 0; j < featureValues[i].size(); ++j) {
				// check if optimisation criterion is violated for one hypothesis and the oracle
				// h(e*) >= h(e_ij) + loss(e_ij)
				// h(e*) - h(e_ij) >= loss(e_ij)
				ScoreComponentCollection featureValueDiff = oracleFeatureValues;
				featureValueDiff.MinusEquals(featureValues[i][j]);
				float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
				if (modelScoreDiff < losses[i][j]) {
					//cerr << "Constraint violated: " << modelScoreDiff << " (modelScoreDiff) < " << losses[i][j] << " (loss)" << endl;
					++numberOfViolatedConstraints;
				}
				else {
					//cerr << "Constraint satisfied: " << modelScoreDiff << " (modelScoreDiff) >= " << losses[i][j] << " (loss)" << endl;
				}

				// Objective: 1/2 * ||w' - w||^2 + C * SUM_1_m[ max_1_n (l_ij - Delta_h_ij.w')]
				// To add a constraint for the optimiser for each sentence i and hypothesis j, we need:
				// 1. vector Delta_h_ij of the feature value differences (oracle - hypothesis)
				// 2. loss_ij - difference in model scores (Delta_h_ij.w') (oracle - hypothesis)
				featureValueDiffs.push_back(featureValueDiff);
				float lossMarginDistance = losses[i][j] - modelScoreDiff;
				lossMarginDistances.push_back(lossMarginDistance);
			}
		}

		if (numberOfViolatedConstraints > 0) {
			// run optimisation
			cerr << "Number of violated constraints: " << numberOfViolatedConstraints << endl;
			// TODO: slack? margin scale factor?
			// compute deltas for all given constraints
			vector< float> deltas = Hildreth::optimise(featureValueDiffs, lossMarginDistances);

			// Update the weight vector according to the deltas and the feature value differences
			// * w' = w' + delta * Dh_ij ---> w' = w' + delta * (h(e*) - h(e_ij))
			for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
				// scale feature value differences with delta
				featureValueDiffs[k].MultiplyEquals(deltas[k]);

				// apply update to weight vector
				currWeights.PlusEquals(featureValueDiffs[k]);
			}
		}
		else {
			cerr << "No constraint violated for this batch" << endl;
		}
	}
	else {
		// SMO:
		for (size_t i = 0; i < featureValues.size(); ++i) {
			// initialise alphas for each source (alpha for oracle translation = C, all other alphas = 0)
			vector< float> alphas(featureValues[i].size());
			for (size_t j = 0; j < featureValues[i].size(); ++j) {
				if (j == m_n) {										// TODO: use oracle index
					// oracle
					alphas[j] = m_c;
					//std::cerr << "alpha " << j << ": " << alphas[j] << endl;
				}
				else {
					alphas[j] = 0;
					//std::cerr << "alpha " << j << ": " << alphas[j] << endl;
				}
			}

			// consider all pairs of hypotheses
			size_t pairs = 0;
			for (size_t j = 0; j < featureValues[i].size(); ++j) {
				for (size_t k = 0; k < featureValues[i].size(); ++k) {
					if (j <= k) {
						++pairs;

						// Compute delta:
						cerr << "\nComparing pair" << j << "," << k << endl;
						ScoreComponentCollection featureValueDiffs;
						float delta = computeDelta(currWeights, featureValues[i], j, k, losses[i], alphas, featureValueDiffs);

						// update weight vector:
						if (delta != 0) {
							update(currWeights, featureValueDiffs, delta);
						}
					}
				}
			}

			cerr << "number of pairs: " << pairs << endl;
		}
	}
}

/*
 * Compute delta for weight update.
 * As part of this compute feature value differences
 * Dh_ij - Dh_ij' ---> h(e_ij') - h(e_ij)) --> h(hope) - h(fear)
 * which are used in the delta term and in the weight update term.
 */
float MiraOptimiser::computeDelta(ScoreComponentCollection& currWeights,
		const vector< ScoreComponentCollection>& featureValues,
		const size_t indexHope,
		const size_t indexFear,
		const vector< float>& losses,
		vector< float>& alphas,
		ScoreComponentCollection& featureValueDiffs) {

	const ScoreComponentCollection featureValuesHope = featureValues[indexHope];		// hypothesis j'
	const ScoreComponentCollection featureValuesFear = featureValues[indexFear];		// hypothesis j

	// compute delta
	float delta = 0.0;
	float diffOfModelScores = 0.0;		// (Dh_ij - Dh_ij') * w' --->  (h(e_ij') - h(e_ij))) * w' (inner product)
	float squaredNorm = 0.0;			// ||Dh_ij - Dh_ij'||^2  --->  sum over squares of elements of h(e_ij') - h(e_ij)

	featureValueDiffs = featureValuesHope;
	featureValueDiffs.MinusEquals(featureValuesFear);
	cerr << "feature value diffs: " << featureValueDiffs << endl;
	squaredNorm = featureValueDiffs.InnerProduct(featureValueDiffs);
	diffOfModelScores = featureValueDiffs.InnerProduct(currWeights);

	if (squaredNorm == 0.0) {
		delta = 0.0;
	}
	else {
		// loss difference used to compute delta: (l_ij - l_ij')  --->  B(e_ij') - B(e_ij)
		// TODO: simplify and use BLEU scores of hypotheses directly?
		float lossDiff = losses[indexFear] - losses[indexHope];
		delta = (lossDiff - diffOfModelScores) / squaredNorm;
		cerr << "delta: " << delta << endl;
		cerr << "loss diff - model diff: " << lossDiff << " - " << diffOfModelScores << endl;

		// clipping
		// fear translation: e_ij  --> alpha_ij  = alpha_ij  + delta
		// hope translation: e_ij' --> alpha_ij' = alpha_ij' - delta
		// clipping interval: [-alpha_ij, alpha_ij']
		// clip delta
		cerr << "Interval [" << (-1 * alphas[indexFear]) << "," << alphas[indexHope] << "]" << endl;
		if (delta > alphas[indexHope]) {
			//cout << "clipping " << delta << " to " << alphas[indexHope] << endl;
			delta = alphas[indexHope];
		}
		else if (delta < (-1 * alphas[indexFear])) {
			//cout << "clipping " << delta << " to " << (-1 * alphas[indexFear]) << endl;
			delta = (-1 * alphas[indexFear]);
		}

		// update alphas
		alphas[indexHope] -= delta;
		alphas[indexFear] += delta;
		//cout << "alpha[" << indexHope << "] = " << alphas[indexHope] << endl;
		//cout << "alpha[" << indexFear << "] = " << alphas[indexFear] << endl;
	}

	return delta;
}

/*
 * Update the weight vector according to delta and the feature value difference
 * w' = w' + delta * (Dh_ij - Dh_ij') ---> w' = w' + delta * (h(e_ij') - h(e_ij)))
 */
void MiraOptimiser::update(ScoreComponentCollection& currWeights, ScoreComponentCollection& featureValueDiffs, const float delta) {
	featureValueDiffs.MultiplyEquals(delta);
	currWeights.PlusEquals(featureValueDiffs);
}

}


