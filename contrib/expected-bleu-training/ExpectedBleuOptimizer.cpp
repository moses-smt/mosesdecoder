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


#include "ExpectedBleuOptimizer.h"


namespace ExpectedBleuTraining
{


void ExpectedBleuOptimizer::AddTrainingInstance(const size_t nBestSizeCount, 
                                                const std::vector<float>& sBleu,
                                                const std::vector<double>& overallScoreUntransformed, 
                                                const std::vector< boost::unordered_map<size_t, float> > &sparseScore,
                                                bool maintainUpdateSet) 
{

  // compute xBLEU
  double sumUntransformedScores = 0.0;
  for (std::vector<double>::const_iterator overallScoreUntransformedIt=overallScoreUntransformed.begin();
       overallScoreUntransformedIt!=overallScoreUntransformed.end(); ++overallScoreUntransformedIt)
  {
    sumUntransformedScores += *overallScoreUntransformedIt;
  }

  double xBleu = 0.0;
  assert(nBestSizeCount == overallScoreUntransformed.size());
  std::vector<double> p;
  for (size_t i=0; i<nBestSizeCount; ++i)
  {
    if (sumUntransformedScores != 0) {
      p.push_back( overallScoreUntransformed[i] / sumUntransformedScores );
    } else {
      p.push_back( 0 );
    }
    xBleu += p.back() * sBleu[ i ];
  }

  for (size_t i=0; i<nBestSizeCount; ++i)
  {
    double D = sBleu[ i ] - xBleu;
    for (boost::unordered_map<size_t, float>::const_iterator sparseScoreIt=sparseScore[i].begin();
         sparseScoreIt!=sparseScore[i].end(); ++sparseScoreIt)
    {
      const size_t name = sparseScoreIt->first;
      float N = sparseScoreIt->second;
      if ( std::fpclassify( p[i] * N * D ) == FP_SUBNORMAL )
      {
        m_err << "Error: encountered subnormal value: p[i] * N * D= " << p[i] * N * D 
              << " with p[i]= " << p[i] << " N= " << N << " D= " << D << '\n';
        m_err.flush();
        exit(1);
      } else {
        m_gradient[name] += p[i] * N * D;
        if ( maintainUpdateSet )
        {
          m_updateSet.insert(name);
        }
      }
    }
  }

  m_xBleu += xBleu;
}


void ExpectedBleuOptimizer::InitSGD(const std::vector<float>& sparseScalingFactor)
{
  const size_t nFeatures = sparseScalingFactor.size();
  memcpy(&m_previousSparseScalingFactor.at(0), &sparseScalingFactor.at(0), nFeatures);
  m_gradient.resize(nFeatures);
}


float ExpectedBleuOptimizer::UpdateSGD(std::vector<float>& sparseScalingFactor, 
                                       size_t batchSize,
                                       bool useUpdateSet)
{

  float xBleu = m_xBleu / batchSize;

  // update sparse scaling factors

  if (useUpdateSet) {

    for (std::set<size_t>::const_iterator it = m_updateSet.begin(); it != m_updateSet.end(); ++it)
    {
      size_t name = *it;
      UpdateSingleScalingFactorSGD(name, sparseScalingFactor, batchSize);
    }
    
    m_updateSet.clear();

  } else {

    for (size_t name=0; name<sparseScalingFactor.size(); ++name)
    {
      UpdateSingleScalingFactorSGD(name, sparseScalingFactor, batchSize);
    }

  }

  m_xBleu = 0;
  m_gradient.clear();
  return xBleu;
}


void ExpectedBleuOptimizer::UpdateSingleScalingFactorSGD(size_t name,
                                                         std::vector<float>& sparseScalingFactor, 
                                                         size_t batchSize)
{
  // regularization
  if ( m_regularizationParameter != 0 )
  {
    m_gradient[name] = m_gradient[name] / m_xBleu - m_regularizationParameter * 2 * sparseScalingFactor[name];
  } else {
    // need to normalize by dividing by batchSize
    m_gradient[name] /= batchSize;
  }

  // the actual update
  sparseScalingFactor[name] += m_learningRate * m_gradient[name];

  // discard scaling factors below a threshold
  if ( fabs(sparseScalingFactor[name]) < m_floorAbsScalingFactor )
  {
    sparseScalingFactor[name] = 0;
  }
}


void ExpectedBleuOptimizer::InitRPROP(const std::vector<float>& sparseScalingFactor)
{
  const size_t nFeatures = sparseScalingFactor.size();
  m_previousSparseScalingFactor.resize(nFeatures);
  memcpy(&m_previousSparseScalingFactor.at(0), &sparseScalingFactor.at(0), nFeatures);
  m_previousGradient.resize(nFeatures);
  m_gradient.resize(nFeatures);
  m_stepSize.resize(nFeatures, m_initialStepSize);
}


float ExpectedBleuOptimizer::UpdateRPROP(std::vector<float>& sparseScalingFactor,
                                         const size_t batchSize)
{

  float xBleu = m_xBleu / batchSize;

  // update sparse scaling factors

  for (size_t name=0; name<sparseScalingFactor.size(); ++name)
  {
    // Sum of gradients. All we need is the sign. Don't need to normalize by dividing by batchSize.

    // regularization
    if ( m_regularizationParameter != 0 )
    {
      m_gradient[name] = m_gradient[name] / m_xBleu - m_regularizationParameter * 2 * sparseScalingFactor[name];
    }

    // step size
    int sign = Sign(m_gradient[name]) * Sign(m_previousGradient[name]);
    if (sign > 0) {
      m_stepSize[name] *= m_increaseRate;
    } else if (sign < 0) {
      m_stepSize[name] *= m_decreaseRate;
    }
    if (m_stepSize[name] < m_minStepSize) {
      m_stepSize[name] = m_minStepSize;
    }
    if (m_stepSize[name] > m_maxStepSize) {
      m_stepSize[name] = m_maxStepSize;
    }

    // the actual update

    m_previousGradient[name] = m_gradient[name];
    if (sign >= 0) {
      if (m_gradient[name] > 0) {
        m_previousSparseScalingFactor[name] = sparseScalingFactor[name];
        sparseScalingFactor[name] += m_stepSize[name];
      } else if (m_gradient[name] < 0) {
        m_previousSparseScalingFactor[name] = sparseScalingFactor[name];
        sparseScalingFactor[name] -= m_stepSize[name];
      }
    } else { 
      sparseScalingFactor[name] = m_previousSparseScalingFactor[name];
      // m_previousGradient[name] = 0;
    }

    // discard scaling factors below a threshold
    if ( fabs(sparseScalingFactor[name]) < m_floorAbsScalingFactor )
    {
      sparseScalingFactor[name] = 0;
    }
  }

  m_xBleu = 0;
  m_gradient.clear();
  return xBleu;
}


}

