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



#pragma once

#include <iostream>
#include <vector>
#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include <boost/shared_ptr.hpp>

#include "FeatureVector.h"

namespace Josiah {
      std::vector<Moses::FValue> hildreth ( const std::vector<Moses::FVector>& a, const std::vector<Moses::FValue>& b );
      std::vector<Moses::FValue> hildreth ( const std::vector<Moses::FVector>& a, const std::vector<Moses::FValue>& b, Moses::FValue );
  
  class OnlineLearner {
    public:
      void virtual doUpdate(
                    const Moses::FVector& currentFV,
                    const Moses::FVector& targetFV,
                    const Moses::FVector& optimaFV,
                    float currentGain,
                    float targetGain,
                    float optimalGain,
                    Moses::FVector& weights) = 0;
      virtual bool usesOptimalSolution() {return false;}
      
      virtual ~OnlineLearner() {}
	
    protected:
  };
  
  class PerceptronLearner : public OnlineLearner {
    public:
      PerceptronLearner();
      void setLearningRate(float learningRate);
      
      void virtual doUpdate(
                    const Moses::FVector& currentFV,
                    const Moses::FVector& targetFV,
                    const Moses::FVector& optimalFV,
                    float currentGain,
                    float targetGain,
                    float optimalGain,
                    Moses::FVector& weights);
      
      private:
        float m_learningRate;
  };
  
  
  
  class MiraLearner : public OnlineLearner {
    public:
      MiraLearner();
      void setSlack(float slack);
      void setMarginScale(float marginScale);
      void setFixMargin(bool fixMargin);
      void setMargin(float margin);
      /** For incorporating gain, an alternative to margin rescaling */
      void setUseSlackRescaling(bool useSlackRescaling);
      void setScaleLossByTargetGain(bool scaleLossByTargetGain);
      
      void virtual doUpdate(
          const Moses::FVector& currentFV,
          const Moses::FVector& targetMFV,
          const Moses::FVector& optimalFV,
          float currentGain,
          float targetGain,
          float optimalGain,
          Moses::FVector& weights);
      
    protected:
      float m_slack;
      float m_marginScale;
      bool m_fixMargin;
      float m_margin;
      bool m_useSlackRescaling;
      bool m_scaleLossByTargetGain;
  };
  
  class MiraPlusLearner : public MiraLearner {
    public:
      void virtual doUpdate(
          const Moses::FVector& currentFV,
          const Moses::FVector& targetFV,
          const Moses::FVector& optimaFV,
          float currentGain,
          float targetGain,
          float optimalGain,
          Moses::FVector& weights);
      virtual bool usesOptimalSolution() {return true;}
  };
  
  typedef boost::shared_ptr<OnlineLearner> OnlineLearnerHandle;
}
