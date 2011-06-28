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

#include <memory>
#include <string>

#include "FeatureVector.h"


namespace Josiah {

/**
  * Singleton to manager weight sets.
**/
class WeightManager {
  public:
    //init from file
    static void init(const std::string& weightsFile);
    //init with zero weights
    static void init();
    static WeightManager& instance();
    
    Moses::FVector& get();
    void scale(Moses::FValue& scale);
    void dump(const std::string& filename);
     
    
  private:
    WeightManager() {}
    static std::auto_ptr<WeightManager> s_instance;
    Moses::FVector m_weights;

};

}
