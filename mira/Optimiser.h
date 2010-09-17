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
#ifndef _MIRA_OPTIMISER_H_
#define _MIRA_OPTIMISER_H_

#include <vector>

#include "ScoreComponentCollection.h"


namespace Mira {
  
  class Optimiser {
    public:
      Optimiser() {}
      virtual float updateWeights(const std::vector<float>& currWeights,
                         const std::vector<const Moses::ScoreComponentCollection*>& scores,
                         const std::vector<float>& losses,
                         const Moses::ScoreComponentCollection oracleScores,
                         std::vector<float>& newWeights) = 0;
  };
 
  class DummyOptimiser : public Optimiser {
    public:
      virtual float updateWeights(const std::vector<float>& currWeights,
                         const std::vector<const Moses::ScoreComponentCollection*>& scores,
                         const std::vector<float>& losses,
                         const Moses::ScoreComponentCollection oracleScores,
                         std::vector<float>& newWeights) {newWeights = currWeights; return 0.0; }
  };

  class MiraOptimiser : public Optimiser {
   public:
     MiraOptimiser(float lowerBound, float upperBound) :
       Optimiser(),
       lowerBound_(lowerBound),
       upperBound_(upperBound) { maxTranslation_ = 0.0; }

     ~MiraOptimiser() {} 
     
    virtual float updateWeights(const std::vector<float>& currWeights,
                         const std::vector<const Moses::ScoreComponentCollection*>& scores,
                         const std::vector<float>& losses,
                         const Moses::ScoreComponentCollection oracleScores,
                         std::vector<float>& newWeights);
   private:
     float lowerBound_;
     float upperBound_;
     float maxTranslation_;
  };
}

#endif
