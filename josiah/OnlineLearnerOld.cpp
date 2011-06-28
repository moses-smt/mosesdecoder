/*
 *  OnlineLearner.cpp
 *  josiah
 *
 *  Created by Abhishek Arun on 26/06/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#ifdef MPI_ENABLED
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
#include "MpiDebug.h"
#endif

#include "OnlineLearner.h"
#include "TranslationDelta.h"
#include "StaticData.h"
#include "Sampler.h"
#include "Gibbler.h"
#include "WeightManager.h"

#ifdef MPI_ENABLED
namespace mpi = boost::mpi;
#endif

namespace Josiah {
  
  FVector OnlineLearner::GetAveragedWeights() { 
    return m_cumulWeights / m_iteration;
  }
  
  void OnlineLearner::UpdateCumul() { 
    m_cumulWeights += GetCurrWeights();
    m_iteration++;
  }

  vector<FValue> OnlineLearner::hildreth (const vector<FVector>& a, const vector<FValue>& b) {
    
    size_t i;
    int max_iter = 10000;
    float eps = 0.00000001;
    float zero = 0.000000000001;
    
    vector<FValue> alpha ( b.size() );
    vector<FValue> F ( b.size() );
    vector<FValue> kkt ( b.size() );
    
    float max_kkt = -1e100;
    
    size_t K = b.size();
    
    float A[K][K];
    bool is_computed[K];
    for ( i = 0; i < K; i++ )
    {
      A[i][i] = a[i].inner_product( a[i]);
      is_computed[i] = false;
    }
    
    int max_kkt_i = -1;
    
    
    for ( i = 0; i < b.size(); i++ )
    {
      F[i] = b[i];
      kkt[i] = F[i];
      if ( kkt[i] > max_kkt )
      {
        max_kkt = kkt[i];
        max_kkt_i = i;
      }
    }
    
    int iter = 0;
    FValue diff_alpha;
    FValue try_alpha;
    FValue add_alpha;
    
    while ( max_kkt >= eps && iter < max_iter )
    {
      
      diff_alpha = A[max_kkt_i][max_kkt_i] <= zero ? 0.0 : F[max_kkt_i]/A[max_kkt_i][max_kkt_i];
      try_alpha = alpha[max_kkt_i] + diff_alpha;
      add_alpha = 0.0;
      
      if ( try_alpha < 0.0 )
        add_alpha = -1.0 * alpha[max_kkt_i];
      else
        add_alpha = diff_alpha;
      
      alpha[max_kkt_i] = alpha[max_kkt_i] + add_alpha;
      
      if ( !is_computed[max_kkt_i] )
      {
        for ( i = 0; i < K; i++ )
        {
          A[i][max_kkt_i] = a[i].inner_product(a[max_kkt_i] ); // for version 1
          //A[i][max_kkt_i] = 0; // for version 1
          is_computed[max_kkt_i] = true;
        }
      }
      
      for ( i = 0; i < F.size(); i++ )
      {
        F[i] -= add_alpha * A[i][max_kkt_i];
        kkt[i] = F[i];
        if ( alpha[i] > zero )
          kkt[i] = abs ( F[i] );
      }
      max_kkt = -1e100;
      max_kkt_i = -1;
      for ( i = 0; i < F.size(); i++ )
        if ( kkt[i] > max_kkt )
        {
          max_kkt = kkt[i];
          max_kkt_i = i;
        }
      
      iter++;
    }
    
    return alpha;
  }
  
  vector<float> OnlineLearner::hildreth (const vector<FVector>& a, const vector<FValue>& b, FValue C) {
    
    size_t i;
    int max_iter = 10000;
    FValue eps = 0.00000001;
    FValue zero = 0.000000000001;
    
    vector<FValue> alpha ( b.size() );
    vector<FValue> F ( b.size() );
    vector<FValue> kkt ( b.size() );
    
    float max_kkt = -1e100;
    
    size_t K = b.size();
    
    float A[K][K];
    bool is_computed[K];
    for ( i = 0; i < K; i++ )
    {
      A[i][i] = a[i].inner_product( a[i]);
      is_computed[i] = false;
    }
    
    int max_kkt_i = -1;
    
    
    for ( i = 0; i < b.size(); i++ )
    {
      F[i] = b[i];
      kkt[i] = F[i];
      if ( kkt[i] > max_kkt )
      {
        max_kkt = kkt[i];
        max_kkt_i = i;
      }
    }
    
    int iter = 0;
    FValue diff_alpha;
    FValue try_alpha;
    FValue add_alpha;
    
    while ( max_kkt >= eps && iter < max_iter )
    {
      
      diff_alpha = A[max_kkt_i][max_kkt_i] <= zero ? 0.0 : F[max_kkt_i]/A[max_kkt_i][max_kkt_i];
      try_alpha = alpha[max_kkt_i] + diff_alpha;
      add_alpha = 0.0;
      
      if ( try_alpha < 0.0 )
        add_alpha = -1.0 * alpha[max_kkt_i];
      else if (try_alpha > C)
				add_alpha = C - alpha[max_kkt_i];
      else
        add_alpha = diff_alpha;
      
      alpha[max_kkt_i] = alpha[max_kkt_i] + add_alpha;
      
      if ( !is_computed[max_kkt_i] )
      {
        for ( i = 0; i < K; i++ )
        {
          A[i][max_kkt_i] = a[i].inner_product(a[max_kkt_i] ); // for version 1
          //A[i][max_kkt_i] = 0; // for version 1
          is_computed[max_kkt_i] = true;
        }
      }
      
      for ( i = 0; i < F.size(); i++ )
      {
        F[i] -= add_alpha * A[i][max_kkt_i];
        kkt[i] = F[i];
        if (alpha[i] > C - zero)
					kkt[i]=-kkt[i];
				else if (alpha[i] > zero)
					kkt[i] = abs(F[i]);
        
      }
      max_kkt = -1e100;
      max_kkt_i = -1;
      for ( i = 0; i < F.size(); i++ )
        if ( kkt[i] > max_kkt )
        {
          max_kkt = kkt[i];
          max_kkt_i = i;
        }
      
      iter++;
    }
    
    return alpha;
  }
  
  
  void PerceptronLearner::doUpdate(const TDeltaHandle& curr, 
                                   const TDeltaHandle& target, 
                                   const TDeltaHandle& noChangeDelta, 
                                   const FVector& optimalFV,
                                   const FValue optimalGain, 
                                   const GainFunctionHandle& gf) {
    //Do update if target gain is better than curr gain
    if (target->GetGain(gf) > curr->GetGain(gf)) {
      GetCurrWeights() -= curr->getScores();
      GetCurrWeights() += target->getScores();  
      m_numUpdates++;
    }
     
    UpdateCumul();
  }
	
  void CWLearner::doUpdate(const TDeltaHandle& curr, 
                           const TDeltaHandle& target, 
                           const TDeltaHandle& noChangeDelta, 
                           const FVector& optimalFV,
                           const FValue optimalGain, 
                           const GainFunctionHandle& gf) {
		//we consider the following binary classification task: does the target jump have a higher gain than the curr jump?

		//the score for the input features (could also be calculated by m_features * current weights)
		float scoreDiff = target->getScore() - curr->getScore();
		VERBOSE(1, "ScoreDiff: " << scoreDiff << endl)
		
		//what is the actual gain of target vs current (the gold gain)
		float gainDiff = target->GetGain(gf) - curr->GetGain(gf); 
		VERBOSE(1, "GainDiff: " << gainDiff << endl)
		
		//the gold 1/-1 label
		float y = gainDiff > 0 ? 1.0 : -1.0;
		VERBOSE(1, "Label: " << y << endl)

		
		//the mean of margin for this task is y * score
		float marginMean = y * scoreDiff;	
		VERBOSE(1, "marginMean: " << marginMean << endl)

		//only update at error
		if (marginMean < 0) {
		
			//the input feature vector to this task is (f(target) - f(curr)) 
            m_features.clear();
			m_features += target->getScores();
			m_features += curr->getScores();

			VERBOSE(1, "feature delta: " << m_features << endl)
			
			//the variance is based on the input features
			float marginVariance = calculateMarginVariance(m_features);
			VERBOSE(1, "marginVariance: " << marginVariance << endl)
		
			//get the kkt multiplier
			float alpha = kkt(marginMean,marginVariance);
			VERBOSE(1, "alpha: " << alpha << endl)
			
			//update the mean parameters
			updateMean(alpha,	y);

			VERBOSE(1, "new weights: " << GetCurrWeights() << endl)
			
			//update the variance parameters
			updateVariance(alpha);
			
			VERBOSE(1, "new variance: " << m_currSigmaDiag << endl)

			
			//remember that we made an update
			m_numUpdates++;

		}
		UpdateCumul();
	}
	
	
  
    void MiraLearner::doUpdate(const TDeltaHandle& curr, 
                               const TDeltaHandle& target, 
                               const TDeltaHandle& noChangeDelta, 
                               const FVector& optimalFV,
                               const FValue optimalGain, 
                               const GainFunctionHandle& gf) {
    
    vector<float> b;
    float scoreDiff = target->getScore() - curr->getScore();
    float gainDiff = target->GetGain(gf) - curr->GetGain(gf); //%BLEU
    
    //Scale the margin
    gainDiff *= m_marginScaleFactor;
  
    //Or set it to a fixed value
    if (m_fixMargin) {
      gainDiff = m_margin;  
    }
    
    
    bool doMira = false;
    if (scoreDiff < gainDiff) { //MIRA Constraints not satisfied, run MIRA
      doMira = true;
      b.push_back( gainDiff - scoreDiff);  
      vector<FVector> distance;
      FVector dist(target->getScores());
      dist -= curr->getScores();
      distance.push_back(dist);    
      
      vector<float> alpha;
      
      if (m_slack == -1)
        alpha = hildreth(distance,b);
      else
        alpha = hildreth(distance,b, m_slack);
      
      FVector update;
      for (size_t k = 0; k < alpha.size(); k++) {
        IFVERBOSE(1) { 
          cerr << "alpha " << alpha[k] << endl;
        }  
        FVector dist = distance[k];
        dist *= alpha[k];
        update += dist;
        m_numUpdates++;
      }
      
      //Normalize
      VERBOSE(1, "Update wv " << update << endl);
      if (m_normalizer)
        m_normalizer->Normalize(update);
      VERBOSE(1,"After norm, Update wv " << update << endl);
      

    }
    else {
      IFVERBOSE(1) { 
        cerr << "Not doing updates cos constraints already satisified" << endl;
        cerr << "Target score" << target->getScore() << ", curr score " << curr->getScore() << endl;
        cerr << "Target (scaled) gain" << target->GetGain(gf) * m_marginScaleFactor << ", curr gain " << curr->GetGain(gf) * m_marginScaleFactor << endl;
      }  
    }
    
    UpdateCumul();
   
    IFVERBOSE(1) { 
    if (doMira) {
      //Sanity check
      curr->updateWeightedScore();
      target->updateWeightedScore();
      
     cerr << "Target score - curr score " << target->getScore() - curr->getScore() << endl;
      cerr << "Target scaled gain - curr scaled gain " << ((target->GetGain(gf) - curr->GetGain(gf)) *  m_marginScaleFactor ) << endl;
    }
    }
  }
  
  /*This Update enforces 3 constraints :
   1. Score of target  - Score of curr >= 1
   2. Score of optimal - Score of Target > 1
   3. Score of optimal - Score of curr > 1
   
  */
  void MiraPlusLearner::doUpdate(const TDeltaHandle& curr, 
                                 const TDeltaHandle& target, 
                                 const TDeltaHandle& noChangeDelta, 
                                 const FVector& optimalFV,
                                 const FValue optimalGain, 
                                 const GainFunctionHandle& gf) {
    
    FVector currFV = curr->getScores();
    currFV -= noChangeDelta->getScores();
    currFV += curr->getSample().GetFeatureValues();
    FValue currScore = currFV.inner_product(WeightManager::instance().get());
    
    FVector targetFV = target->getScores();
    targetFV -= noChangeDelta->getScores();
    targetFV += target->getSample().GetFeatureValues();
    FValue targetScore = targetFV.inner_product(WeightManager::instance().get());
    
    FValue optimalGainScore = optimalFV.inner_product(WeightManager::instance().get());

    float targetGain = target->GetGain(gf);
    float currGain = curr->GetGain(gf);
    
    IFVERBOSE(1) {
      cerr << "Optimal deriv has (scaled) gain " << m_marginScaleFactor * optimalGain << " , fv : " << optimalFV << " [ " << optimalGainScore << " ]" << endl;
      cerr << "target deriv has (scaled) gain " << m_marginScaleFactor * targetGain << " , fv : " << targetFV << " [ " << targetScore << " ]" << endl;
      cerr << "curr deriv has (scaled) gain " << m_marginScaleFactor * currGain << " , fv : " << currFV << " [ " << currScore << " ]" << endl;
    }
    
    vector<float> b;
    vector<FVector> distance;
    
    float tgtcurrmargin = m_marginScaleFactor * (targetGain - currGain);
    float opttgtmargin = m_marginScaleFactor * (optimalGain - targetGain);
    float optcurrmargin = m_marginScaleFactor * (optimalGain - currGain);
    
    if (m_fixMargin) {
      tgtcurrmargin = m_margin;
      opttgtmargin = m_margin;
      optcurrmargin = m_margin;
    }
    
    
    //Score of target - Score of curr >= 1
    b.push_back(tgtcurrmargin - (targetScore - currScore));
    distance.push_back(targetFV);
    distance.back() -= currFV;

    VERBOSE(2, cerr << "b[0] = " << b[0] << " distance[0] = " << distance[0] << endl);
    
    
    if (optimalGain > targetGain) {
      //Score of optimal - Score of Target > 1
      b.push_back(opttgtmargin - (optimalGainScore - targetScore));
      distance.push_back(optimalFV);
      distance.back() -= targetFV;
      VERBOSE(2, cerr << "b[1] = " << b[1] << " distance[1] = " << distance[1] << endl);
      
      //Score of optimal - Score of curr > 1 
      b.push_back(optcurrmargin - (optimalGainScore - currScore));
      distance.push_back(optimalFV);
      distance.back() -= currFV;
      VERBOSE(2, cerr << "b[2] = " << b[2] << " distance[2] = " << distance[2] << endl);
    }
    
    
    vector<float> alpha;
    if (m_slack == -1)
      alpha = hildreth(distance,b);
    else
      alpha = hildreth(distance,b, m_slack);
    
    FVector update;
    for (size_t k = 0; k < alpha.size(); k++) {
      IFVERBOSE(1) {
        cerr << "alpha " << alpha[k] << endl;
        cerr << "dist " << distance[k] << endl;
      }
      distance[k] *= alpha[k];
      update += distance[k];
    }
    
    //Normalize
    VERBOSE(2, "Before normalise: " << update << endl);
    if (m_normalizer)
      m_normalizer->Normalize(update);
    VERBOSE(2, "After normalise: " << update << endl);
    GetCurrWeights() += update;
    IFVERBOSE(1) {
      cerr << "Mira++ updated weights to " << GetCurrWeights() << endl;
    }
    
    m_numUpdates++;
    UpdateCumul();
    
    IFVERBOSE(1) { 
      //Sanity check
      currScore = currFV.inner_product(WeightManager::instance().get());
      targetScore = targetFV.inner_product(WeightManager::instance().get());
      optimalGainScore = optimalFV.inner_product(WeightManager::instance().get());
      
      cerr << "Updated Current Weights : " << GetCurrWeights() << endl;
      
      cerr << "Target score - curr score " << targetScore - currScore << endl;
      cerr << "margin * (Target gain - curr gain) " << m_marginScaleFactor * (targetGain - currGain) << endl;
      
      cerr << "Optimal score - target score " << optimalGainScore - targetScore << endl;
      cerr << "margin * (Optimal gain - target gain) " << m_marginScaleFactor * (optimalGain - targetGain) << endl;
      
      cerr << "Optimal score - curr score " << optimalGainScore - currScore << endl;
      cerr << "margin * (Optimal gain - curr gain) " << m_marginScaleFactor * (optimalGain - currGain) << endl;
    }
  }
}
