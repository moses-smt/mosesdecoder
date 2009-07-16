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

namespace Josiah {
  
  ScoreComponentCollection OnlineLearner::GetAveragedWeights() { 
    ScoreComponentCollection averagedWgts(m_cumulWeights);
    averagedWgts.DivideEquals(m_iteration);
    return averagedWgts; 
  }
  
  void PerceptronLearner::doUpdate(TranslationDelta* curr, TranslationDelta* target) {
    //Do update if target gain is better than curr gain
    if (target->getGain() > curr->getGain()) {
      m_currWeights.MinusEquals(curr->getScores());
      m_currWeights.PlusEquals(target->getScores());  
    }

    m_cumulWeights.PlusEquals(m_currWeights);
    m_iteration++;
    //cerr << "Curr Weights : " << m_currWeights << endl;
    const_cast<StaticData&>(StaticData::Instance()).SetAllWeights(m_currWeights.data());
  }
  
  void MiraLearner::doUpdate(TranslationDelta* curr, TranslationDelta* target) {
    
    vector<float> b;
    float scoreDiff = target->getScore() - curr->getScore();
    float gainDiff = (target->getGain() - curr->getGain()) * 100; //%BLEU
    
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
        ScoreComponentCollection dist = distance[k];
        dist.MultiplyEquals(alpha[k]);
        m_currWeights.PlusEquals(dist);
      }
    }
    else {
      IFVERBOSE(1) { 
        cerr << "Not doing updates cos constraints already satisified" << endl;
        cerr << "Target score" << target->getScore() << ", curr score " << curr->getScore() << endl;
        cerr << "Target gain" << target->getGain() << ", curr gain " << curr->getGain() << endl;
      }  
    }
    
    m_cumulWeights.PlusEquals(m_currWeights);
    m_iteration++;
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
  
  vector<float> MiraLearner::hildreth (const vector<ScoreComponentCollection>& a, const vector<float>& b) {
    
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
  
}
