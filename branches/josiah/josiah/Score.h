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

#include "ScoreComponentCollection.h"
#include "StaticData.h"

namespace Josiah {

/**
 * Set of weights, including the moses weights.
 **/
  class WeightVector {
    public:
      WeightVector();
      float getWeight(const std::string& name) const;
      void setScore(const std::string& name, float value);
      std::vector<float> getWeights();
      
      friend std::ostream& operator<<(std::ostream&, const WeightVector&);
      
  };


  /**
    * Set of scores, including the moses scores.
   **/
  class ScoreVector {
    
    public:
      ScoreVector(const Moses::ScoreComponentCollection& mosesScores);
      
      float getScore(const std::string& name) const;
      void setScore(const std::string& name, float value);
      std::vector<float> getScores();
      Moses::ScoreComponentCollection& getMosesScores();
      ScoreVector operator+(const ScoreVector rhs) const;
      void operator+=(const ScoreVector rhs);
      float innerProduct(const WeightVector& weights);
      
      friend std::ostream& operator<<(std::ostream&, const ScoreVector&);
    
    
    private:
      std::map<std::string,float> m_scores;
  };
  
  


}//namespace
