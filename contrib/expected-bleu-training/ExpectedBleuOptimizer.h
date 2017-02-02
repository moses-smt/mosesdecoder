/*
   Moses - statistical machine translation system
   Copyright (C) 2005-2015 University of Edinburgh

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
*/


#pragma once

#include <vector>
#include <set>
#include <boost/unordered_map.hpp>
#include "util/file_stream.hh"


namespace ExpectedBleuTraining
{

class ExpectedBleuOptimizer
{
public:

  ExpectedBleuOptimizer(util::FileStream& err,
                        float learningRate=1,
                        float initialStepSize=0.001,
                        float decreaseRate=0.5,
                        float increaseRate=1.2,
                        float minStepSize=1e-7,
                        float maxStepSize=1,
                        float floorAbsScalingFactor=0,
                        float regularizationParameter=0)
    : m_err(err)
    , m_learningRate(learningRate)
    , m_initialStepSize(initialStepSize)
    , m_decreaseRate(decreaseRate)
    , m_increaseRate(increaseRate)
    , m_minStepSize(minStepSize)
    , m_maxStepSize(maxStepSize)
    , m_floorAbsScalingFactor(floorAbsScalingFactor)
    , m_regularizationParameter(regularizationParameter)
    , m_xBleu(0)
  { }

  void AddTrainingInstance(const size_t nBestSizeCount, 
                           const std::vector<float>& sBleu,
                           const std::vector<double>& overallScoreUntransformed, 
                           const std::vector< boost::unordered_map<size_t, float> > &sparseScore,
                           bool maintainUpdateSet = false); 

  void InitSGD(const std::vector<float>& sparseScalingFactor);

  float UpdateSGD(std::vector<float>& sparseScalingFactor, 
                  size_t batchSize,
                  bool useUpdateSet = false);

  void InitRPROP(const std::vector<float>& sparseScalingFactor);
  
  float UpdateRPROP(std::vector<float>& sparseScalingFactor,
                    const size_t batchSize);

protected:

  util::FileStream& m_err;

  // for SGD
  const float m_learningRate;

  // for RPROP
  const float m_initialStepSize;
  const float m_decreaseRate;
  const float m_increaseRate;
  const float m_minStepSize;
  const float m_maxStepSize;

  std::vector<float> m_previousSparseScalingFactor;
  std::vector<float> m_previousGradient;
  std::vector<float> m_gradient;
  std::vector<float> m_stepSize;

  // other
  const float m_floorAbsScalingFactor;
  const float m_regularizationParameter;

  double m_xBleu;

  std::set<size_t> m_updateSet;


  void UpdateSingleScalingFactorSGD(size_t name,
                                    std::vector<float>& sparseScalingFactor, 
                                    size_t batchSize);


  inline int Sign(double x) 
  {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
  }
};

}


