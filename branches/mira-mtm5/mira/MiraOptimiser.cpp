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
		size_t violatedConstraintsBefore = 0;
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
				cerr << "loss of hypothesis: " << losses[i][j] << endl;
				cerr << "model score difference: " << modelScoreDiff << endl;
				float loss = losses[i][j] * m_marginScaleFactor;

				bool addConstraint = true;
				if (modelScoreDiff < loss) {
					// constraint violated
					++violatedConstraintsBefore;
				}
				else if (m_onlyViolatedConstraints) {
					// constraint not violated
					addConstraint = false;
				}

				if (addConstraint) {
					// Objective: 1/2 * ||w' - w||^2 + C * SUM_1_m[ max_1_n (l_ij - Delta_h_ij.w')]
					// To add a constraint for the optimiser for each sentence i and hypothesis j, we need:
					// 1. vector Delta_h_ij of the feature value differences (oracle - hypothesis)
					// 2. loss_ij - difference in model scores (Delta_h_ij.w') (oracle - hypothesis)
					featureValueDiffs.push_back(featureValueDiff);
					float lossMarginDistance = loss - modelScoreDiff;
					lossMarginDistances.push_back(lossMarginDistance);
				}
			}
		}

		if (violatedConstraintsBefore > 0) {
			// TODO: slack?
			// run optimisation
			cerr << "\nNumber of violated constraints: " << violatedConstraintsBefore << endl;
			// compute deltas for all given constraints
			vector< float> deltas = Hildreth::optimise(featureValueDiffs, lossMarginDistances);

			// Update the weight vector according to the deltas and the feature value differences
			// * w' = w' + delta * Dh_ij ---> w' = w' + delta * (h(e*) - h(e_ij))
			for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
				cerr << "delta: " << deltas[k] << endl;

				// compute update
				featureValueDiffs[k].MultiplyEquals(deltas[k]);

				// apply update to weight vector
				currWeights.PlusEquals(featureValueDiffs[k]);
			}

			// sanity check: how many constraints violated after optimisation?
			size_t violatedConstraintsAfter = 0;
			for (size_t i = 0; i < featureValues.size(); ++i) {
				for (size_t j = 0; j < featureValues[i].size(); ++j) {
					ScoreComponentCollection featureValueDiff = oracleFeatureValues;
					featureValueDiff.MinusEquals(featureValues[i][j]);
					float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
					float loss = losses[i][j] * m_marginScaleFactor;
					if (modelScoreDiff < loss) {
						++violatedConstraintsAfter;
					}

					cerr << "New model score difference: " << modelScoreDiff << endl;
				}
			}

			cerr << "Number of violated constraints after optimisation: " << violatedConstraintsAfter << endl;
			if (violatedConstraintsAfter > violatedConstraintsBefore) {
				cerr << "Increase: " << violatedConstraintsAfter - violatedConstraintsBefore << endl << endl;
			}
		}
		else {
			cerr << "No constraint violated for this batch" << endl;
		}
	}
	else {
		// SMO:
		for (size_t i = 0; i < featureValues.size(); ++i) {
			vector< float> alphas(featureValues[i].size()); // TODO: dont pass alphas if not needed
			if (!m_fixedClipping) {
				// initialise alphas for each source (alpha for oracle translation = C, all other alphas = 0)
				for (size_t j = 0; j < featureValues[i].size(); ++j) {
					if (j == m_oracleIndex) {
						// oracle
						alphas[j] = m_c;
						//std::cerr << "alpha " << j << ": " << alphas[j] << endl;
					}
					else {
						alphas[j] = 0;
						//std::cerr << "alpha " << j << ": " << alphas[j] << endl;
					}
				}
			}

			// consider all pairs of hypotheses
			size_t pairs = 0;
			for (size_t j = 0; j < featureValues[i].size(); ++j) {
				for (size_t k = 0; k < featureValues[i].size(); ++k) {
					if (j <= k) {
						++pairs;

						// Compute delta:
						ScoreComponentCollection featureValueDiffs;
						float delta = computeDelta(currWeights, featureValues[i], j, k, losses[i], alphas, featureValueDiffs);

						// update weight vector:
						if (delta != 0) {
							update(currWeights, featureValueDiffs, delta);
							cerr << "\nComparing pair" << j << "," << k << endl;
							cerr << "Update with delta: " << delta << endl;
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
		const size_t j,
		const size_t k,
		const vector< float>& losses,
		vector< float>& alphas,
		ScoreComponentCollection& featureValueDiffs) {

 	const ScoreComponentCollection featureValuesJ = featureValues[j]; // hypothesis j'
 	const ScoreComponentCollection featureValuesK = featureValues[k]; // hypothesis j

 	// compute delta
 	float delta = 0.0;
 	float diffOfModelScores = 0.0; // (Dh_ij - Dh_ij') * w' ---> (h(e_ij') - h(e_ij))) * w' (inner product)
 	float squaredNorm = 0.0; // ||Dh_ij - Dh_ij'||^2 ---> sum over squares of elements of h(e_ij') - h(e_ij)

 	featureValueDiffs = featureValuesJ;
 	featureValueDiffs.MinusEquals(featureValuesK);
 	squaredNorm = featureValueDiffs.InnerProduct(featureValueDiffs);
 	diffOfModelScores = featureValueDiffs.InnerProduct(currWeights);

 	if (squaredNorm == 0.0) {
 		delta = 0.0;
 	}
 	else {
 		// loss difference used to compute delta: (l_ij - l_ij') ---> B(e_ij') - B(e_ij)
 		// TODO: simplify and use BLEU scores of hypotheses directly?
 		float lossDiff = losses[k] - losses[j];
 		delta = (lossDiff - diffOfModelScores) / squaredNorm;

 		// clipping
 		if (m_fixedClipping) {
 			if (delta > m_c) {
 				delta = m_c;
 			}
 			else if (delta < -1 * m_c) {
 				delta = -1 * m_c;
 			}
 		}
 		else {
 			// alpha_ij = alpha_ij + delta
 			// alpha_ij' = alpha_ij' - delta
 			// clipping interval: [-alpha_ij, alpha_ij']
 			// clip delta
 			if (delta > alphas[j]) {
 				delta = alphas[j];
 			}
 			else if (delta < (-1 * alphas[k])) {
 				delta = (-1 * alphas[k]);
 			}

 			// update alphas
 			alphas[j] -= delta;
 			alphas[k] += delta;
 		}
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


