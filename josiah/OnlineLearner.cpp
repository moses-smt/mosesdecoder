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


#ifdef MPI_ENABLED
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>
#endif

#include "OnlineLearner.h"
#include "Utils.h"

#ifdef MPI_ENABLED
namespace mpi = boost::mpi;
#endif

using namespace std;

namespace Josiah {
  
  
  PerceptronLearner::PerceptronLearner() :
      m_learningRate(1.0) {}
  
  void PerceptronLearner::setLearningRate(float learningRate) {
    m_learningRate = learningRate;
  }
  
  void  PerceptronLearner::doUpdate(
      const FVector& currentFV,
      const FVector& targetFV,
      const FVector&,
      float currentGain,
      float targetGain,
      float,
      FVector& weights) 
  {
    //Do update if target gain is better than curr gain
    if (targetGain > currentGain) {
      weights -= m_learningRate * currentFV;
      weights += m_learningRate * targetFV;  
    }
  }
  
  MiraLearner::MiraLearner() :
      m_slack(0),
      m_marginScale(1),
      m_fixMargin(false),
      m_margin(1),
      m_useSlackRescaling(false) {}
  
  void MiraLearner::setSlack(float slack) {
    m_slack = slack;
  }
  
  void MiraLearner::setMarginScale(float marginScale) {
    m_marginScale = marginScale;
  }
  
  void MiraLearner::setFixMargin(bool fixMargin) {
    m_fixMargin = fixMargin;
  }
  
  void MiraLearner::setMargin(float margin) {
    m_margin = margin;
  }

  void MiraLearner::setUseSlackRescaling(bool useSlackRescaling) {
    m_useSlackRescaling = useSlackRescaling;
  }

  void MiraLearner::setScaleLossByTargetGain(bool scaleLossByTargetGain) {
    m_scaleLossByTargetGain = scaleLossByTargetGain;
  }
  
  void  MiraLearner::doUpdate(
      const FVector& currFV,
      const FVector& targetFV,
      const FVector& ,
      float currGain,
      float targetGain,
      float ,
      FVector& weights)
  {
    FValue currScore = currFV.inner_product(weights);
    FValue targetScore = targetFV.inner_product(weights);
    
    VERBOSE(1,"currGain: " << currGain << " targetGain " << targetGain << endl);
    
    IFVERBOSE(1) {
      cerr << "target deriv has (scaled) gain " << m_marginScale * targetGain << " , fv : " << targetFV << " [ " << targetScore << " ]" << endl;
      cerr << "curr deriv has (scaled) gain " << m_marginScale * currGain << " , fv : " << currFV << " [ " << currScore << " ]" << endl;
    }
    
    float loss = m_marginScale * (targetGain - currGain);
    if (m_scaleLossByTargetGain) {
      loss = loss*targetGain;
    }
    
    float margin;
    if (m_fixMargin || m_useSlackRescaling) {
      margin = m_margin;
    } else {
      margin =  loss;
    }
    
    //constraint is a.x >= b, where x is the change in weight vector
    float b = margin - (targetScore - currScore);
    if (b<= 0) {
      VERBOSE(1, "MiraLearner: alpha = " << 0  << endl);
      return;
    }
    FVector a = targetFV - currFV;
    float norma =inner_product(a,a);
    if (norma == 0) {
      VERBOSE(1, "MiraLearner: alpha = " << 0  << endl);
      return;
    }
    
    //Update is min(C , b / ||a||^2) a
    //where C is slack
    //See Crammer et al, passive-aggressive paper for soln of similar problem.
    float alpha = b / norma;
    float slack = m_slack;
    if (m_useSlackRescaling) {
      slack = slack * loss;
    }
    VERBOSE(1, "MiraLearner: b = " << b << " norma = " << norma <<
      " unclipped alpha = " << alpha << endl);
    if (slack && alpha > slack) {
      alpha = slack;
    }
    
    VERBOSE(1, "MiraLearner: alpha = " << alpha << endl);
    weights += alpha * a;
  }
      
  void  MiraPlusLearner::doUpdate(
      const FVector& currFV,
      const FVector& targetFV,
      const FVector& optimalFV,
      float currGain,
      float targetGain,
      float optimalGain,
      FVector& weights)
  {
    FValue currScore = currFV.inner_product(weights);
    FValue targetScore = targetFV.inner_product(weights);
    FValue optimalGainScore = optimalFV.inner_product(weights);

    VERBOSE(1,"currGain: " << currGain << " targetGain " << targetGain << " optimalGain " << optimalGain << endl);
    
    cerr << "currGain: " << currGain << " targetGain " << targetGain << " optimalGain " << optimalGain << endl;
    cerr << "currScore: " << currScore << " targetScore " << targetScore << " optimalScore " << optimalGainScore << endl;
    
    
    IFVERBOSE(1) {
      cerr << "Optimal deriv has (scaled) gain " << m_marginScale * optimalGain << " , fv : " << optimalFV << " [ " << optimalGainScore << " ]" << endl;
      cerr << "target deriv has (scaled) gain " << m_marginScale * targetGain << " , fv : " << targetFV << " [ " << targetScore << " ]" << endl;
      cerr << "curr deriv has (scaled) gain " << m_marginScale * currGain << " , fv : " << currFV << " [ " << currScore << " ]" << endl;
    }
    
    vector<float> b;
    vector<FVector> distance;
    
    float tgtcurrmargin = m_marginScale * (targetGain - currGain);
    float opttgtmargin = m_marginScale * (optimalGain - targetGain);
    float optcurrmargin = m_marginScale * (optimalGain - currGain);
    
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

    /*
    cerr << "b " << b[0];
    if (b.size() > 1) {
      cerr << " " << b[1] << " " << b[2];
    }
    cerr << endl;
    */
    
    
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

    
    cerr << "alpha " << alpha[0];
    if (alpha.size() > 1) {
      cerr << " " << alpha[1] << " " << alpha[2];
    }
    cerr << endl;
    cerr << update << endl;
    
    
    
    weights += update;
    IFVERBOSE(1) {
      cerr << "Mira++ updated weights to " << weights << endl;
    }
    
    IFVERBOSE(1) { 
      //Sanity check
      currScore = currFV.inner_product(weights);
      targetScore = targetFV.inner_product(weights);
      optimalGainScore = optimalFV.inner_product(weights);
      
      cerr << "Updated Current Weights : " << weights << endl;
      
      cerr << "Target score - curr score " << targetScore - currScore << endl;
      cerr << "margin * (Target gain - curr gain) " << m_marginScale * (targetGain - currGain) << endl;
      
      cerr << "Optimal score - target score " << optimalGainScore - targetScore << endl;
      cerr << "margin * (Optimal gain - target gain) " << m_marginScale * (optimalGain - targetGain) << endl;
      
      cerr << "Optimal score - curr score " << optimalGainScore - currScore << endl;
      cerr << "margin * (Optimal gain - curr gain) " << m_marginScale * (optimalGain - currGain) << endl;
    }
  }
  
  

  vector<FValue> hildreth (const vector<FVector>& a, const vector<FValue>& b) {
    
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
  
  vector<FValue> hildreth (const vector<FVector>& a, const vector<FValue>& b, FValue C) {
    
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
  
  
  
}
