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

#include <stdexcept>
#include <fstream>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include "ReorderingFeature.h"

#include "Gibbler.h"
#include "Util.h"

using namespace Moses;
using namespace std;
using boost::lexical_cast;

namespace Josiah {

  FeatureFunctionHandle ReorderingFeature::getFunction(const Sample& sample) const
  {
    return FeatureFunctionHandle(
      new ReorderingFeatureFunction(sample,m_mosesFeature));
  }

  ReorderingFeatureFunction::ReorderingFeatureFunction(
    const Sample& sample,
    const DiscriminativeReorderingFeature& mosesFeature) :
      FeatureFunction(sample),
      m_mosesFeature(mosesFeature) {}

  void ReorderingFeatureFunction::assignScore(FVector& scores) {
    //Use the score cached by updateTarget() 
    scores += m_accumulator.GetScoresVector();
  }

  void ReorderingFeatureFunction::updateTarget() {
    //update all the previous states, and the accumulator
    m_prevStates.clear();
    m_accumulator.ZeroAll();
    const Hypothesis* currHypo = getSample().GetTargetTail();
  }
}
