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

#include "FeatureFunction.h"

namespace Josiah {

auto_ptr<FeatureRegistry> FeatureRegistry::s_instance;

const FeatureRegistry* FeatureRegistry::instance() {
  if (!s_instance.get()) {
    s_instance.reset(new FeatureRegistry());
  }
  return s_instance.get();
}

FeatureRegistry::FeatureRegistry() {
  m_names.push_back("dummy");
}


const std::vector<string>& FeatureRegistry::getFeatureNames() const {
  return m_names;
}


void FeatureRegistry::createFeatures(Sample& sample, std::vector<FeatureFunction*> features) {
  features.push_back(new DummyFeatureFunction(sample));
}

} //namespace
