/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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

#include <map>
#include <string>
#include <vector>

#include "FeatureFunction.h"
#include "ScoreComponentCollection.h"
#include "StaticData.h"

namespace Josiah {


class FeatureRegistry;


//TODO: Could be templated
class NamedValues {
  public:
    const float& operator[](const std::string& name) const;
    float& operator[](const std::string& name);
    
  private:
    static float s_default;
    std::map<std::string,float> m_namedValues;
};

/**
 * Set of weights, including the moses weights.
 **/
  class WeightVector : public NamedValues{
    public:
      WeightVector();
      /** All weights */
      std::vector<float> getAllWeights() const;
      const std::vector<float>& getMosesWeights() const;
      
      //friend std::ostream& operator<<(std::ostream&, const WeightVector&);
      
    private:
      
  };
  
  std::ostream& operator<<(std::ostream&, const WeightVector&);


  /**
    * Set of scores, including the moses scores.
   **/
  class ScoreVector : public NamedValues{
    
    public:
      ScoreVector(const Moses::ScoreComponentCollection& mosesScores) :
        m_mosesScores(mosesScores) {}
      //Creates a zero vector
      ScoreVector() {}
      /** All scores */
      std::vector<float> getAllScores() const;
      Moses::ScoreComponentCollection getMosesScores() const {return m_mosesScores;}
      Moses::ScoreComponentCollection& getMosesScores()  {return m_mosesScores;}
      ScoreVector operator+(const ScoreVector& rhs) const;
      ScoreVector operator-(const ScoreVector& rhs) const;
      void operator+=(const ScoreVector& rhs);
      void operator-=(const ScoreVector& rhs);
      /** Inner product */
      float operator*(const WeightVector& weights) const;
      
    private:
      Moses::ScoreComponentCollection m_mosesScores;
  };
  
  std::ostream& operator<<(std::ostream&, const ScoreVector&);
  


}//namespace
