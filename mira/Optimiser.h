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
      virtual void updateWeights(Moses::ScoreComponentCollection& weights,
                         const std::vector<std::vector<Moses::ScoreComponentCollection> >& scores,
                         const std::vector<std::vector<float> >& losses,
                         const Moses::ScoreComponentCollection& oracleScores) = 0;
  };
 
  class DummyOptimiser : public Optimiser {
    public:
      virtual void updateWeights(Moses::ScoreComponentCollection& weights,
                         const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
                         const std::vector< std::vector<float> >& losses,
                         const Moses::ScoreComponentCollection& oracleScores) 
                         {/* do nothing */}
  };
 
  class Perceptron : public Optimiser {
    public:
       

      virtual void updateWeights(Moses::ScoreComponentCollection& weights,
                         const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
                         const std::vector< std::vector<float> >& losses,
                         const Moses::ScoreComponentCollection& oracleScores);
  };

  class MiraOptimiser : public Optimiser {
   public:
	  MiraOptimiser() :
		  Optimiser() { }

	  MiraOptimiser(size_t n, bool hildreth, float marginScaleFactor, bool onlyViolatedConstraints, float clipping, bool fixedClipping) :
		  Optimiser(),
		  m_n(n),
		  m_hildreth(hildreth),
		  m_marginScaleFactor(marginScaleFactor),
		  m_onlyViolatedConstraints(onlyViolatedConstraints),
		  m_c(clipping),
		  m_fixedClipping(fixedClipping) { }

     ~MiraOptimiser() {}
   
      virtual void updateWeights(Moses::ScoreComponentCollection& weights,
      						  const std::vector< std::vector<Moses::ScoreComponentCollection> >& scores,
      						  const std::vector< std::vector<float> >& losses,
      						  const Moses::ScoreComponentCollection& oracleScores);
      float computeDelta(Moses::ScoreComponentCollection& currWeights,
      				const std::vector< Moses::ScoreComponentCollection>& featureValues,
      				const size_t indexHope,
      				const size_t indexFear,
      				const std::vector< float>& losses,
      				std::vector< float>& alphas,
      				Moses::ScoreComponentCollection& featureValueDiffs);
      void update(Moses::ScoreComponentCollection& currWeights, Moses::ScoreComponentCollection& featureValueDiffs, const float delta);

      void setOracleIndex(size_t oracleIndex) {
    	  m_oracleIndex = oracleIndex;
      }
  
   private:
      // number of hypotheses used for each nbest list (number of hope, fear, best model translations)
      size_t m_n;

      // whether or not to use the Hildreth algorithm in the optimisation step
      bool m_hildreth;

      // scaling the margin to regularise updates
      float m_marginScaleFactor;

      // add only violated constraints to the optimisation problem
      bool m_onlyViolatedConstraints;

      // clipping threshold for SMO to regularise updates
      float m_c;

      // use a fixed clipping threshold with SMO  (instead of adaptive)
      bool m_fixedClipping;

      // index of oracle translation in hypothesis matrix
      size_t m_oracleIndex;
  };
}

#endif
