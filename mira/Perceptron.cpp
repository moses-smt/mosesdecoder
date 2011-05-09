/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "Optimiser.h"

using namespace Moses;
using namespace std;

namespace Mira {

  vector<int> Perceptron::updateWeightsAnalytically(ScoreComponentCollection& currWeights,
						       ScoreComponentCollection& featureValues,
						       float loss,
						       ScoreComponentCollection& oracleFeatureValues,
						       float oracleBleuScore,
						       size_t sentenceId,
						       float learning_rate,
						       float max_sentence_update,
						       size_t rank,
						       size_t epoch,
						       bool controlUpdates) {
    vector<int> status(1);
    status[0] = 0;
    return status;
  }

vector<int> Perceptron::updateWeights(ScoreComponentCollection& currWeights,
		const vector< vector<ScoreComponentCollection> >& featureValues,
		const vector< vector<float> >& losses,
		const vector< vector<float> >& bleuScores,
		const vector< ScoreComponentCollection>& oracleFeatureValues,
		const vector< float> oracleBleuScores,
		const vector< size_t> dummy,
		float learning_rate,
		float max_sentence_update,
		size_t rank,
		size_t epoch,
		int updates_per_epoch,
		bool controlUpdates)
{
	for (size_t i = 0; i < featureValues.size(); ++i) {
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			if (losses[i][j] > 0) {
				currWeights.MinusEquals(featureValues[i][j]);
				currWeights.PlusEquals(oracleFeatureValues[i]);
			}
		}
	}

	vector<int> status(1);
	status[0] = 0;
	return status;
}
}

