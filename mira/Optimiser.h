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
      virtual std::vector<int> updateWeights(Moses::ScoreComponentCollection& weights,
            						  const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValues,
            						  const std::vector< std::vector<float> >& losses,
            						  const std::vector<std::vector<float> >& bleuScores,
            						  const std::vector< Moses::ScoreComponentCollection>& oracleFeatureValues,
            						  const std::vector< float> oracleBleuScores,
            						  const std::vector< size_t> sentenceId,
      										float learning_rate,
      										float max_sentence_update,
      										size_t rank,
													size_t epoch,
      										int updates_per_epoch,
      										bool controlUpdates) = 0;
  };
 
  class Perceptron : public Optimiser {
    public:
			virtual std::vector<int> updateWeights(Moses::ScoreComponentCollection& weights,
                         const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValues,
                         const std::vector< std::vector<float> >& losses,
                         const std::vector<std::vector<float> >& bleuScores,
                         const std::vector<Moses::ScoreComponentCollection>& oracleFeatureValues,
                         const std::vector< float> oracleBleuScores,
                         const std::vector< size_t> dummy,
                         float learning_rate,
                         float max_sentence_update,
                         size_t rank,
                         size_t epoch,
                         int updates_per_epoch,
                         bool controlUpdates);
  };

  class MiraOptimiser : public Optimiser {
   public:
	  MiraOptimiser() :
		  Optimiser() { }

	  MiraOptimiser(size_t n, bool hildreth, float marginScaleFactor, bool onlyViolatedConstraints, float slack, bool weightedLossFunction, size_t maxNumberOracles, bool accumulateMostViolatedConstraints, bool pastAndCurrentConstraints, size_t exampleSize) :
		  Optimiser(),
		  m_n(n),
		  m_hildreth(hildreth),
		  m_marginScaleFactor(marginScaleFactor),
		  m_onlyViolatedConstraints(onlyViolatedConstraints),
		  m_slack(slack),
		  m_weightedLossFunction(weightedLossFunction),
		  m_max_number_oracles(maxNumberOracles),
		  m_accumulateMostViolatedConstraints(accumulateMostViolatedConstraints),
		  m_pastAndCurrentConstraints(pastAndCurrentConstraints),
		  m_oracles(exampleSize),
		  m_bleu_of_oracles(exampleSize) { }

     ~MiraOptimiser() {}
   
     virtual std::vector<int> updateWeights(Moses::ScoreComponentCollection& weights,
      						  const std::vector< std::vector<Moses::ScoreComponentCollection> >& featureValues,
      						  const std::vector< std::vector<float> >& losses,
      						  const std::vector<std::vector<float> >& bleuScores,
      						  const std::vector< Moses::ScoreComponentCollection>& oracleFeatureValues,
      						  const std::vector< float> oracleBleuScores,
      						  const std::vector< size_t> sentenceId,
										float learning_rate,
										float max_sentence_update,
										size_t rank,
										size_t epoch,
										int updates_per_epoch,
										bool controlUpdates);

      void setOracleIndices(std::vector<size_t> oracleIndices) {
    	  m_oracleIndices= oracleIndices;
      }

      void setSlack(float slack) {
      	m_slack = slack;
      }

      void setMarginScaleFactor(float msf) {
      	m_marginScaleFactor = msf;
      }

      Moses::ScoreComponentCollection getAccumulatedUpdates() {
					return m_accumulatedUpdates;
      }

      void resetAccumulatedUpdates() {
      	m_accumulatedUpdates.ZeroAll();
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

      // regularise Hildreth updates
      float m_slack;

      bool m_weightedLossFunction;

      // index of oracle translation in hypothesis matrix
      std::vector<size_t> m_oracleIndices;

      // keep a list of oracle translations over epochs
      std::vector < std::vector< Moses::ScoreComponentCollection> > m_oracles;

      std::vector < std::vector< float> > m_bleu_of_oracles;

      size_t m_max_number_oracles;

      // accumulate most violated constraints for every example
      std::vector< Moses::ScoreComponentCollection> m_featureValueDiffs;
      std::vector< float> m_lossMarginDistances;

      bool m_accumulateMostViolatedConstraints;

      bool m_pastAndCurrentConstraints;

      Moses::ScoreComponentCollection m_accumulatedUpdates;
  };
}

#endif
