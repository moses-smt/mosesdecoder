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

void Perceptron::updateWeights(ScoreComponentCollection& currWeights,
                   const vector< vector<const ScoreComponentCollection*> >& scores,
                   const vector<vector<float> >& losses,
                   const ScoreComponentCollection& oracleScores)
{
  for (size_t i = 0; i < scores.size(); ++i) {
    for (size_t j = 0; j < scores[i].size(); ++j) {
      if (losses[i][j] > 0) {
        currWeights.MinusEquals(*scores[i][j]);
        currWeights.PlusEquals(oracleScores);      
      }
    }
  }
}

}

