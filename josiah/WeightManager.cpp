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
 
#include <cassert>
#include <stdexcept>

#include "StaticData.h"
#include "WeightManager.h"

using namespace Moses;
using namespace std;

namespace Josiah {
  
  auto_ptr<WeightManager> WeightManager::s_instance;

  void WeightManager::init(const std::string& weightsFile) {
    init();
    s_instance->m_weights.load(weightsFile);
  }
  
  void WeightManager::init() {
    assert(!s_instance.get());
    s_instance.reset(new WeightManager());
  }
  
  WeightManager& WeightManager::instance() {
    assert(s_instance.get());
    return *s_instance;
  }

  FVector& WeightManager::get() {
    return m_weights;
  }
  
  void WeightManager::scale(FValue& scale) {
    m_weights *= scale;
  }
  
  void WeightManager::dump(const string& filename) {
    ofstream out(filename.c_str());
    if (!out) {
        cerr << "WARN: Failed to open " << filename << " for weight dump" << endl;
    } else {
            m_weights.write(out);
            out.close();
    }
  }
  

}
