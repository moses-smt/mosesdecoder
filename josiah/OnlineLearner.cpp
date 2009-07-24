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
  
  void MiraLearner::doUpdate(TranslationDelta* curr, TranslationDelta* target, TranslationDelta* noChangeDelta, Sampler& sampler) {
    
    vector<float> b;
    float scoreDiff = target->getScore() - curr->getScore();
    float gainDiff = target->getGain() - curr->getGain(); //%BLEU
    gainDiff = 1; // Enforce this margin
    
    bool doMira = false;
    if (scoreDiff < gainDiff) { //MIRA Constraints not satisfied, run MIRA
      doMira = true;
      b.push_back( gainDiff - scoreDiff);  
      vector<ScoreComponentCollection> distance;
      ScoreComponentCollection dist(target->getScores());
      dist.MinusEquals(curr->getScores());
      distance.push_back(dist);    
      
      vector<float> alpha = hildreth(distance,b);
      
      for (size_t k = 0; k < alpha.size(); k++) {
        IFVERBOSE(1) { 
          cerr << "alpha " << alpha[k] << endl;
        }  
        ScoreComponentCollection dist = distance[k];
        dist.MultiplyEquals(alpha[k]);
        m_currWeights.PlusEquals(dist);
        m_numUpdates++;
      }
    }
    else {
      IFVERBOSE(1) { 
        cerr << "Not doing updates cos constraints already satisified" << endl;
        cerr << "Target score" << target->getScore() << ", curr score " << curr->getScore() << endl;
        cerr << "Target gain" << target->getGain() << ", curr gain " << curr->getGain() << endl;
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
      cerr << "Target gain - curr gain " << target->getGain() - curr->getGain() << endl;
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
      cerr << "Optimal deriv has gain " << sampler.GetOptimalGain() << " , fv : " << OptimalGainFV << " [ " << optimalGainScore << " ]" << endl;
      cerr << "target deriv has gain " << target->getGain() << " , fv : " << targetFV << " [ " << targetScore << " ]" << endl;
      cerr << "curr deriv has gain " << curr->getGain() << " , fv : " << currFV << " [ " << currScore << " ]" << endl;
    }
    
    vector<float> b;
    vector<ScoreComponentCollection> distance;
    
    float margin = 1;
    
    //Score of target - Score of curr >= 1
    b.push_back(margin - (targetScore - currScore));
    ScoreComponentCollection dist(targetFV);
    dist.MinusEquals(currFV);
    distance.push_back(dist);    
    
    
    if (sampler.GetOptimalGain() > target->getGain()) {
      //Score of optimal - Score of Target > 1
      b.push_back(margin - (optimalGainScore - targetScore));
      dist = OptimalGainFV;
      dist.MinusEquals(targetFV);
      distance.push_back(dist);    
      
      //Score of optimal - Score of curr > 1 
      b.push_back(margin - (optimalGainScore - currScore));
      dist = OptimalGainFV;
      dist.MinusEquals(currFV);
      distance.push_back(dist);      
    }
    
    
    vector<float> alpha = hildreth(distance,b);
    
    for (size_t k = 0; k < alpha.size(); k++) {
      ScoreComponentCollection dist = distance[k];
      dist.MultiplyEquals(alpha[k]);
      m_currWeights.PlusEquals(dist);
      IFVERBOSE(1) {
        cerr << "alpha " << alpha[k] << endl;
        cerr << "dist " << dist << endl;
        cerr << "Curr Weights " << m_currWeights << endl;
      }
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
      cerr << "Target gain - curr gain " << target->getGain() - curr->getGain() << endl;
      
      cerr << "Optimal score - target score " << optimalGainScore - targetScore << endl;
      cerr << "Optimal gain - target gain " << sampler.GetOptimalGain() - target->getGain() << endl;
      
      cerr << "Optimal score - curr score " << optimalGainScore - currScore << endl;
      cerr << "Optimal gain - curr gain " << sampler.GetOptimalGain() - curr->getGain() << endl;
    }
  }
}
