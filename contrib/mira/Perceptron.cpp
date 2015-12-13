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

namespace Mira
{

size_t Perceptron::updateWeightsHopeFear(
  ScoreComponentCollection& weightUpdate,
  const vector< vector<ScoreComponentCollection> >& featureValuesHope,
  const vector< vector<ScoreComponentCollection> >& featureValuesFear,
  const vector< vector<float> >& dummy1,
  const vector< vector<float> >& dummy2,
  const vector< vector<float> >& dummy3,
  const vector< vector<float> >& dummy4,
  float perceptron_learning_rate,
  size_t rank,
  size_t epoch,
  int updatePosition)
{
  cerr << "Rank " << rank << ", epoch " << epoch << ", hope: " << featureValuesHope[0][0] << endl;
  cerr << "Rank " << rank << ", epoch " << epoch << ", fear: " << featureValuesFear[0][0] << endl;
  ScoreComponentCollection featureValueDiff = featureValuesHope[0][0];
  featureValueDiff.MinusEquals(featureValuesFear[0][0]);
  cerr << "Rank " << rank << ", epoch " << epoch << ", hope - fear: " << featureValueDiff << endl;
  featureValueDiff.MultiplyEquals(perceptron_learning_rate);
  weightUpdate.PlusEquals(featureValueDiff);
  cerr << "Rank " << rank << ", epoch " << epoch << ", update: " << featureValueDiff << endl;
  return 0;
}

}

