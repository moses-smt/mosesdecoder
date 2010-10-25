#include "Optimiser.h"

using namespace Moses;
using namespace std;

namespace Mira {

void MiraOptimiser::updateWeights(ScoreComponentCollection& currWeights,
		const vector< vector<ScoreComponentCollection> >& featureValues,
		const vector< vector<float> >& losses,
		const ScoreComponentCollection& oracleFeatureValues) {

	// TODO: do we need the oracle feature values?

	size_t numberOfUpdates = 0;

	vector< float> alphas(3*m_n);
	for(size_t batch = 0; batch < featureValues.size(); batch++) {
		if (m_clippingScheme == 2) {
			// initialise alphas for each source (alpha for oracle translation = C, all other alphas = 0)
			for (size_t j = 0; j < 3*m_n; ++j) {
				if (j == m_n) {
					// oracle
					alphas[j] = m_upperBound;
					std::cout << "alpha " << j << ": " << alphas[j] << endl;
				}
				else {
					alphas[j] = 0;
					std::cout << "alpha " << j << ": " << alphas[j] << endl;
				}
			}
		}

		// iterate over nbest lists of translations, feature list contains n*model, n*hope, n*fear)
		// Combinations for j and j': hope/fear, hope/model, model/fear?
		// Currently we compare each hope against each fear (10x10),
		// each hope against each model (10x10), each model against each fear translation (10x10)
		for (size_t j = 0; j < m_n; ++j) {
			size_t indexModel_j = j;
			size_t indexHope_j = j + m_n;		// e_ij'
			size_t indexFear_j = j + 2*m_n;	// e_ij

			for (size_t k = 0; k < m_n; ++k) {
				size_t indexModel_k = k;
				size_t indexHope_k = k + m_n;		// e_ij'
				size_t indexFear_k = k + 2*m_n;	// e_ij

				// Hypothesis pair hope/fear
				// Compute delta:
				cout << "\nComparing hope/fear (" << indexHope_j << "," << indexFear_k << ")" << endl;
				ScoreComponentCollection featureValueDiffs;
				float delta = computeDelta(currWeights, featureValues[batch], indexHope_j, indexFear_k, losses[batch], alphas, featureValueDiffs);

				// update weight vector:
				if (delta != 0) {
					update(currWeights, featureValueDiffs, delta);
					++numberOfUpdates;
				}

				// Hypothesis pair hope/model
				// Compute delta:
				cout << "\nComparing hope/model (" << indexHope_j << "," << indexModel_k << ")" << endl;
				featureValueDiffs.ZeroAll();
				delta = computeDelta(currWeights, featureValues[batch], indexHope_j, indexModel_k, losses[batch], alphas, featureValueDiffs);

				// update weight vector:
				if (delta != 0) {
					update(currWeights, featureValueDiffs, delta);
					++numberOfUpdates;
				}

				// Hypothesis pair model/fear
				// Compute delta:
				cout << "\nComparing model/fear (" << indexModel_j << "," << indexFear_k << ")" << endl;
				featureValueDiffs.ZeroAll();
				delta = computeDelta(currWeights, featureValues[batch], indexModel_j, indexFear_k, losses[batch], alphas, featureValueDiffs);

				// update weight vector:
				if (delta != 0) {
					update(currWeights, featureValueDiffs, delta);
					++numberOfUpdates;
				}
			}

			cout << endl;
		}
	}

	cout << "Number of updates: " << numberOfUpdates << endl;
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
	cout << "feature value diffs: " << featureValueDiffs << endl;
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
		cout << "delta: " << delta << endl;
		cout << "loss diff - model diff: " << lossDiff << " - " << diffOfModelScores << endl;

		// clipping
		switch (m_clippingScheme) {
		case 1:
			if (delta > m_upperBound) {
				cout << "clipping " << delta << " to " << m_upperBound << endl;
				delta = m_upperBound;
			}
			else if (delta < m_lowerBound) {
				cout << "clipping " << delta << " to " << m_lowerBound << endl;
				delta = m_lowerBound;
			}

			// TODO: update
			//m_lowerBound += delta;
			//m_upperBound -= delta;
			//cout << "m_lowerBound = " << m_lowerBound << endl;
			//cout << "m_upperBound = " << m_upperBound << endl;

			break;
		case 2:
			// fear translation: e_ij  --> alpha_ij  = alpha_ij  + delta
			// hope translation: e_ij' --> alpha_ij' = alpha_ij' - delta
			// clipping interval: [-alpha_ij, alpha_ij']
			// clip delta
			cout << "Interval [" << (-1 * alphas[indexFear]) << "," << alphas[indexHope] << "]" << endl;
			if (delta > alphas[indexHope]) {
				cout << "clipping " << delta << " to " << alphas[indexHope] << endl;
				delta = alphas[indexHope];
			}
			else if (delta < (-1 * alphas[indexFear])) {
				cout << "clipping " << delta << " to " << (-1 * alphas[indexFear]) << endl;
				delta = (-1 * alphas[indexFear]);
			}

			// update alphas
			alphas[indexHope] -= delta;
			alphas[indexFear] += delta;
			cout << "alpha[" << indexHope << "] = " << alphas[indexHope] << endl;
			cout << "alpha[" << indexFear << "] = " << alphas[indexFear] << endl;
			break;
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

