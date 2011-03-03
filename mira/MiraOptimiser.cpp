#include "Optimiser.h"
#include "Hildreth.h"

using namespace Moses;
using namespace std;

namespace Mira {

int MiraOptimiser::updateWeights(ScoreComponentCollection& currWeights,
		const vector< vector<ScoreComponentCollection> >& featureValues,
		const vector< vector<float> >& losses,
		const vector<std::vector<float> >& bleuScores,
		const vector< ScoreComponentCollection>& oracleFeatureValues,
		const vector< float> oracleBleuScores,
		const vector< size_t> sentenceIds) {

	// add every oracle in batch to list of oracles (under certain conditions)
	for (size_t i = 0; i < oracleFeatureValues.size(); ++i) {
		float newWeightedScore = oracleFeatureValues[i].GetWeightedScore();
		size_t sentenceId = sentenceIds[i];

		// compare new oracle with existing oracles:
		// if same translation exists, just update the bleu score
		// if not, add the oracle
		bool updated = false;
		size_t indexOfWorst = 0;
		float worstWeightedScore = 0;
		for (size_t j = 0; j < m_oracles[sentenceId].size(); ++j) {
			float currentWeightedScore = m_oracles[sentenceId][j].GetWeightedScore();
			if (currentWeightedScore == newWeightedScore) {
				cerr << "updated.." << endl;
				m_bleu_of_oracles[sentenceId][j] = oracleBleuScores[j];
				updated = true;
				break;
			}
			else if (worstWeightedScore == 0 || currentWeightedScore > worstWeightedScore){
				worstWeightedScore = currentWeightedScore;
				indexOfWorst = j;
			}
		}

		if (!updated) {
			// add if number of maximum oracles not exceeded, otherwise override the worst
			if (m_max_number_oracles > m_oracles[sentenceId].size()) {
				m_oracles[sentenceId].push_back(oracleFeatureValues[i]);
				m_bleu_of_oracles[sentenceId].push_back(oracleBleuScores[i]);
			}
			else {
				m_oracles[sentenceId][indexOfWorst] = oracleFeatureValues[i];
				m_bleu_of_oracles[sentenceId][indexOfWorst] = oracleBleuScores[i];
			}
		}
	}

	size_t violatedConstraintsBefore = 0;
	vector< ScoreComponentCollection> featureValueDiffs;
	vector< float> lossMarginDistances;

	// find most violated constraint
	float maxViolationLossMarginDistance;
	ScoreComponentCollection maxViolationfeatureValueDiff;

	float oldDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValues.size(); ++i) {
		size_t sentenceId = sentenceIds[i];
		if (m_oracles[sentenceId].size() > 1)
			cerr << "Available oracles for source sentence " << sentenceId << ": " << m_oracles[sentenceId].size() << endl;
			for (size_t j = 0; j < featureValues[i].size(); ++j) {
			// check if optimisation criterion is violated for one hypothesis and the oracle
			// h(e*) >= h(e_ij) + loss(e_ij)
			// h(e*) - h(e_ij) >= loss(e_ij)

			// iterate over all available oracles (1 if not accumulating, otherwise one per started epoch)
			for (size_t k = 0; k < m_oracles[sentenceId].size(); ++k) {
				//cerr << "Oracle " << k << ": " << m_oracles[sentenceId][k] << " (BLEU: " << m_bleu_of_oracles[sentenceId][k] << ", model score: " <<  m_oracles[sentenceId][k].GetWeightedScore() << ")" << endl;
				ScoreComponentCollection featureValueDiff = m_oracles[sentenceId][k];
				featureValueDiff.MinusEquals(featureValues[i][j]);
				float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
				float loss = losses[i][j] * m_marginScaleFactor;
				if (m_weightedLossFunction) {
					loss *= log10(bleuScores[i][j]);
				}

				bool addConstraint = true;
				if (modelScoreDiff < loss) {
					// constraint violated
					++violatedConstraintsBefore;
					oldDistanceFromOptimum += (loss - modelScoreDiff);
				}
				else if (m_onlyViolatedConstraints) {
					// constraint not violated
					addConstraint = false;
				}

				if (addConstraint) {
					float lossMarginDistance = loss - modelScoreDiff;

					if (m_accumulateMostViolatedConstraints && !m_pastAndCurrentConstraints) {
						if (lossMarginDistance > maxViolationLossMarginDistance) {
							maxViolationLossMarginDistance = lossMarginDistance;
							maxViolationfeatureValueDiff = featureValueDiff;
						}
					}
					else if (m_pastAndCurrentConstraints) {
						if (lossMarginDistance > maxViolationLossMarginDistance) {
							maxViolationLossMarginDistance = lossMarginDistance;
							maxViolationfeatureValueDiff = featureValueDiff;
						}

						featureValueDiffs.push_back(featureValueDiff);
						lossMarginDistances.push_back(lossMarginDistance);
					}
					else {
						// Objective: 1/2 * ||w' - w||^2 + C * SUM_1_m[ max_1_n (l_ij - Delta_h_ij.w')]
						// To add a constraint for the optimiser for each sentence i and hypothesis j, we need:
						// 1. vector Delta_h_ij of the feature value differences (oracle - hypothesis)
						// 2. loss_ij - difference in model scores (Delta_h_ij.w') (oracle - hypothesis)
						featureValueDiffs.push_back(featureValueDiff);
						lossMarginDistances.push_back(lossMarginDistance);
					}
				}
			}
		}
	}

	if (m_max_number_oracles == 1) {
		for (size_t k = 0; k < sentenceIds.size(); ++k) {
			size_t sentenceId = sentenceIds[k];
			m_oracles[sentenceId].clear();
		}
	}

	cerr << "Number of violated constraints before optimisation: " << violatedConstraintsBefore << endl;
	if (featureValueDiffs.size() != 30) {
		cerr << "Number of constraints passed to optimiser: " << featureValueDiffs.size() << endl;
	}

	// run optimisation: compute alphas for all given constraints
	vector< float> alphas;
	if (m_accumulateMostViolatedConstraints && !m_pastAndCurrentConstraints) {
		m_featureValueDiffs.push_back(maxViolationfeatureValueDiff);
		m_lossMarginDistances.push_back(maxViolationLossMarginDistance);

		if (m_slack != 0) {
			alphas = Hildreth::optimise(m_featureValueDiffs, m_lossMarginDistances, m_slack);
		}
		else {
			alphas = Hildreth::optimise(m_featureValueDiffs, m_lossMarginDistances);
		}

		// Update the weight vector according to the alphas and the feature value differences
		// * w' = w' + delta * Dh_ij ---> w' = w' + delta * (h(e*) - h(e_ij))
		for (size_t k = 0; k < m_featureValueDiffs.size(); ++k) {
			// compute update
			float update = alphas[k];
			m_featureValueDiffs[k].MultiplyEquals(update);

			// apply update to weight vector
			currWeights.PlusEquals(m_featureValueDiffs[k]);
		}
	}
	else if (violatedConstraintsBefore > 0) {
		if (m_pastAndCurrentConstraints) {
			// add all (most violated) past constraints to the list of current constraints
			for (size_t i = 0; i < m_featureValueDiffs.size(); ++i) {
				featureValueDiffs.push_back(m_featureValueDiffs[i]);
				lossMarginDistances.push_back(m_lossMarginDistances[i]);
			}

			// add new most violated constraint to list
			m_featureValueDiffs.push_back(maxViolationfeatureValueDiff);
			m_lossMarginDistances.push_back(maxViolationLossMarginDistance);
		}

		if (m_slack != 0) {
			alphas = Hildreth::optimise(featureValueDiffs, lossMarginDistances, m_slack);
		}
		else {
			alphas = Hildreth::optimise(featureValueDiffs, lossMarginDistances);
		}

		// Update the weight vector according to the alphas and the feature value differences
		// * w' = w' + delta * Dh_ij ---> w' = w' + delta * (h(e*) - h(e_ij))
		for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
			// compute update
			float alpha = alphas[k];
			featureValueDiffs[k].MultiplyEquals(alpha);

			// apply update to weight vector
			currWeights.PlusEquals(featureValueDiffs[k]);
		}
	}
	else {
		cerr << "No constraint violated for this batch" << endl;
		return 0;
	}

	// sanity check: how many constraints violated after optimisation?
	size_t violatedConstraintsAfter = 0;
	float newDistanceFromOptimum = 0;
	for (size_t i = 0; i < featureValues.size(); ++i) {
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			ScoreComponentCollection featureValueDiff = oracleFeatureValues[i];
			featureValueDiff.MinusEquals(featureValues[i][j]);
			float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
			float loss = losses[i][j] * m_marginScaleFactor;
			if (modelScoreDiff < loss) {
				++violatedConstraintsAfter;
				newDistanceFromOptimum += (loss - modelScoreDiff);
			}
		}
	}

	int constraintChange = violatedConstraintsBefore - violatedConstraintsAfter;
	cerr << "Constraint change: " << constraintChange << endl;
	float distanceChange = oldDistanceFromOptimum - newDistanceFromOptimum;
	cerr << "Distance change: " << distanceChange << endl;
	if (constraintChange < 0 && distanceChange < 0) {
		return -1;
	}

	return 0;
}


}


