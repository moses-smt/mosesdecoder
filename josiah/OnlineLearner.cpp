/*
 *  OnlineLearner.cpp
 *  josiah
 *
 *  Created by Abhishek Arun on 26/06/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "OnlineLearner.h"
#include "TranslationDelta.h"
#include "StaticData.h"
#include "Sampler.h"
#include "Gibbler.h"
#ifdef MPI_ENABLED
#include <mpi.h>
#include "MpiDebug.h"
#endif

namespace Josiah {
  
  ScoreComponentCollection OnlineLearner::GetAveragedWeights() { 
    ScoreComponentCollection averagedWgts(m_cumulWeights);
    averagedWgts.DivideEquals(m_iteration);
    return averagedWgts; 
  }
  
  void OnlineLearner::UpdateCumul() { 
    m_cumulWeights.PlusEquals(m_currWeights);
    m_iteration++;
  }
#ifdef MPI_ENABLED  
  void OnlineLearner::SetRunningWeightVector(int rank, int num_procs) {
    vector <float> runningWeights(m_currWeights.size());
    //Reduce running weight vector
    if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&m_currWeights.data()[0]), &runningWeights[0], runningWeights.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
   
    MPI_VERBOSE(1,"Rank " << rank << ", My weights : " << m_currWeights << endl)  
    MPI_VERBOSE(1,"Rank " << rank << ", Agg weights : " << ScoreComponentCollection(runningWeights) << endl)  

    if (rank == 0) {
      ScoreComponentCollection avgRunningWeights(runningWeights);
      avgRunningWeights.DivideEquals(num_procs);
      runningWeights = avgRunningWeights.data();
      MPI_VERBOSE(1,"Avg weights : " << avgRunningWeights << endl)  
    } 
     
    if (MPI_SUCCESS != MPI_Bcast(const_cast<float*>(&runningWeights[0]), runningWeights.size(), MPI_FLOAT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    
    ScoreComponentCollection avgRunningWeights(runningWeights);
    m_currWeights = avgRunningWeights;
    MPI_VERBOSE(1,"Rank " << rank << ", upd curr weights : " << m_currWeights << endl)  
    
    if (MPI_SUCCESS != MPI_Barrier(MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    
    const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(m_currWeights.data());
  }
#endif  

  vector<float> OnlineLearner::hildreth (const vector<ScoreComponentCollection>& a, const vector<float>& b) {
    
    size_t i;
    int max_iter = 10000;
    float eps = 0.00000001;
    float zero = 0.000000000001;
    
    vector<float> alpha ( b.size() );
    vector<float> F ( b.size() );
    vector<float> kkt ( b.size() );
    
    float max_kkt = -1e100;
    
    size_t K = b.size();
    
    float A[K][K];
    bool is_computed[K];
    for ( i = 0; i < K; i++ )
    {
      A[i][i] = a[i].InnerProduct ( a[i]);
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
    float diff_alpha;
    float try_alpha;
    float add_alpha;
    
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
          A[i][max_kkt_i] = a[i].InnerProduct (a[max_kkt_i] ); // for version 1
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
  
  vector<float> OnlineLearner::hildreth (const vector<ScoreComponentCollection>& a, const vector<float>& b, float C) {
    
    size_t i;
    int max_iter = 10000;
    float eps = 0.00000001;
    float zero = 0.000000000001;
    
    vector<float> alpha ( b.size() );
    vector<float> F ( b.size() );
    vector<float> kkt ( b.size() );
    
    float max_kkt = -1e100;
    
    size_t K = b.size();
    
    float A[K][K];
    bool is_computed[K];
    for ( i = 0; i < K; i++ )
    {
      A[i][i] = a[i].InnerProduct ( a[i]);
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
    float diff_alpha;
    float try_alpha;
    float add_alpha;
    
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
          A[i][max_kkt_i] = a[i].InnerProduct (a[max_kkt_i] ); // for version 1
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
  
  
  void PerceptronLearner::doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler) {
    //Do update if target gain is better than curr gain
    if (target->getGain() > curr->getGain()) {
      m_currWeights.MinusEquals(curr->getScores());
      m_currWeights.PlusEquals(target->getScores());  
      m_numUpdates++;
    }
     
    UpdateCumul();
    //cerr << "Curr Weights : " << m_currWeights << endl;
    const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(m_currWeights.data());
  }
	
  void CWLearner::doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler) {
		//we consider the following binary classification task: does the target jump have a higher gain than the curr jump?

		//the score for the input features (could also be calculated by m_features * current weights)
		float scoreDiff = target->getScore() - curr->getScore();
		
		//what is the actual gain of target vs current (the gold gain)
		float gainDiff = target->getGain() - curr->getGain(); 
		
		//the gold 1/-1 label
		float y = gainDiff > 0 ? 1.0 : -1.0;
		
		//the mean of margin for this task is y * score
		float marginMean = y * scoreDiff;	

		//only update at error
		if (marginMean < 0) {
		
			//the input feature vector to this task is (f(target) - f(curr)) 
			m_features.ZeroAll();
			m_features.PlusEquals(target->getScores());
			m_features.MinusEquals(curr->getScores());
				
			//the variance is based on the input features
			float marginVariance = calculateMarginVariance(m_features);
		
			//get the kkt multiplier
			float alpha = kkt(marginMean,marginVariance);
			
			//update the mean parameters
			updateMean(alpha,	y);
			
			//update the variance parameters
			updateVariance(alpha);
			
			//remember that we made an update
			m_numUpdates++;

		}
		UpdateCumul();
    //cerr << "Curr Weights : " << m_currWeights << endl;
    const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(m_currWeights.data());
		
		
		
	}
	
	
  
  void MiraLearner::doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler) {
    
    vector<float> b;
    float scoreDiff = target->getScore() - curr->getScore();
    float gainDiff = target->getGain() - curr->getGain(); //%BLEU
    
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
      vector<ScoreComponentCollection> distance;
      ScoreComponentCollection dist(target->getScores());
      dist.MinusEquals(curr->getScores());
      distance.push_back(dist);    
      
      vector<float> alpha;
      
      if (m_slack == -1)
        alpha = hildreth(distance,b);
      else
        alpha = hildreth(distance,b, m_slack);
      
      ScoreComponentCollection update;
      for (size_t k = 0; k < alpha.size(); k++) {
        IFVERBOSE(1) { 
          cerr << "alpha " << alpha[k] << endl;
        }  
        ScoreComponentCollection dist = distance[k];
        dist.MultiplyEquals(alpha[k]);
        update.PlusEquals(dist);
        m_numUpdates++;
      }
      
      //Normalize
      cerr << "Update wv " << update << endl;
      if (m_normalizer)
        m_normalizer->Normalize(update);
      cerr << "After norm, Update wv " << update << endl;
      

    }
    else {
      IFVERBOSE(1) { 
        cerr << "Not doing updates cos constraints already satisified" << endl;
        cerr << "Target score" << target->getScore() << ", curr score " << curr->getScore() << endl;
        cerr << "Target (scaled) gain" << target->getGain() * m_marginScaleFactor << ", curr gain " << curr->getGain() * m_marginScaleFactor << endl;
      }  
    }
    
    UpdateCumul();
    const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(m_currWeights.data());
   
    IFVERBOSE(1) { 
    if (doMira) {
      //Sanity check
      curr->updateWeightedScore();
      target->updateWeightedScore();
      
     cerr << "Target score - curr score " << target->getScore() - curr->getScore() << endl;
      cerr << "Target scaled gain - curr scaled gain " << ((target->getGain() - curr->getGain()) *  m_marginScaleFactor ) << endl;
    }
    }
  }
  
  /*This Update enforces 3 constraints :
   1. Score of target  - Score of curr >= 1
   2. Score of optimal - Score of Target > 1
   3. Score of optimal - Score of curr > 1
   
  */
  void MiraPlusLearner::doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler) {
    
    ScoreComponentCollection currFV = curr->getScores();
    currFV.MinusEquals(noChangeDelta->getScores());
    currFV.PlusEquals(curr->getSample().GetFeatureValues());
    float currScore = currFV.InnerProduct(StaticData::Instance().GetAllWeights());
    
    ScoreComponentCollection targetFV = target->getScores();
    targetFV.MinusEquals(noChangeDelta->getScores());
    targetFV.PlusEquals(target->getSample().GetFeatureValues());
    float targetScore = targetFV.InnerProduct(StaticData::Instance().GetAllWeights());
    
    const ScoreComponentCollection & OptimalGainFV = sampler.GetOptimalGainSol();
    float optimalGainScore = OptimalGainFV.InnerProduct(StaticData::Instance().GetAllWeights());
    
    IFVERBOSE(1) {
      cerr << "Optimal deriv has (scaled) gain " << m_marginScaleFactor * sampler.GetOptimalGain() << " , fv : " << OptimalGainFV << " [ " << optimalGainScore << " ]" << endl;
      cerr << "target deriv has (scaled) gain " << m_marginScaleFactor * target->getGain() << " , fv : " << targetFV << " [ " << targetScore << " ]" << endl;
      cerr << "curr deriv has (scaled) gain " << m_marginScaleFactor * curr->getGain() << " , fv : " << currFV << " [ " << currScore << " ]" << endl;
    }
    
    vector<float> b;
    vector<ScoreComponentCollection> distance;
    
    float tgtcurrmargin = m_marginScaleFactor * (target->getGain() - curr->getGain());
    float opttgtmargin = m_marginScaleFactor * (sampler.GetOptimalGain() - target->getGain());
    float optcurrmargin = m_marginScaleFactor * (sampler.GetOptimalGain() - curr->getGain());
    
    if (m_fixMargin) {
      tgtcurrmargin = m_margin;
      opttgtmargin = m_margin;
      optcurrmargin = m_margin;
    }
    
    
    //Score of target - Score of curr >= 1
    b.push_back(tgtcurrmargin - (targetScore - currScore));
    ScoreComponentCollection dist(targetFV);
    dist.MinusEquals(currFV);
    distance.push_back(dist);    
    
    
    if (sampler.GetOptimalGain() > target->getGain()) {
      //Score of optimal - Score of Target > 1
      b.push_back(opttgtmargin - (optimalGainScore - targetScore));
      dist = OptimalGainFV;
      dist.MinusEquals(targetFV);
      distance.push_back(dist);    
      
      //Score of optimal - Score of curr > 1 
      b.push_back(optcurrmargin - (optimalGainScore - currScore));
      dist = OptimalGainFV;
      dist.MinusEquals(currFV);
      distance.push_back(dist);      
    }
    
    
    vector<float> alpha;
    if (m_slack == -1)
      alpha = hildreth(distance,b);
    else
      alpha = hildreth(distance,b, m_slack);
    
    ScoreComponentCollection update;
    for (size_t k = 0; k < alpha.size(); k++) {
      ScoreComponentCollection dist = distance[k];
      dist.MultiplyEquals(alpha[k]);
      update.PlusEquals(dist);
      IFVERBOSE(1) {
        cerr << "alpha " << alpha[k] << endl;
        cerr << "dist " << dist << endl;
      }
    }
    
    //Normalize
    if (m_normalizer)
      m_normalizer->Normalize(update);
    m_currWeights.PlusEquals(update);
    IFVERBOSE(1) {
      cerr << "Curr Weights " << m_currWeights << endl;
    }
    
    m_numUpdates++;
    UpdateCumul();
    const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(m_currWeights.data());
    
    IFVERBOSE(1) { 
      //Sanity check
      currScore = currFV.InnerProduct(StaticData::Instance().GetAllWeights());
      targetScore = targetFV.InnerProduct(StaticData::Instance().GetAllWeights());
      optimalGainScore = OptimalGainFV.InnerProduct(StaticData::Instance().GetAllWeights());
      
      cerr << "Curr Weights : " << StaticData::Instance().GetWeights() << endl;
      
      cerr << "Target score - curr score " << targetScore - currScore << endl;
      cerr << "Target gain - curr gain " << m_marginScaleFactor * (target->getGain() - curr->getGain()) << endl;
      
      cerr << "Optimal score - target score " << optimalGainScore - targetScore << endl;
      cerr << "Optimal gain - target gain " << m_marginScaleFactor * (sampler.GetOptimalGain() - target->getGain()) << endl;
      
      cerr << "Optimal score - curr score " << optimalGainScore - currScore << endl;
      cerr << "Optimal gain - curr gain " << m_marginScaleFactor * (sampler.GetOptimalGain() - curr->getGain()) << endl;
    }
  }
}
