// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/StaticData.h"
#include "moses/InputType.h"
#include "moses/TranslationOption.h"
#include "moses/UserMessage.h"

using namespace std;

namespace Moses
{

PhraseDictionary::PhraseDictionary(const std::string &description, const std::string &line)
  :DecodeFeature(description, line)
  ,m_tableLimit(20) // default
{
  size_t ind = 0;
  while (ind < m_args.size()) {
    vector<string> &args = m_args[ind];
    bool consumed = SetParameter(args[0], args[1]);
    if (consumed) {
      m_args.erase(m_args.begin() + ind);
    } else {
      ++ind;
    }
  }
}


const TargetPhraseCollection *PhraseDictionary::
GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const
{
  Phrase phrase = src.GetSubString(range);
  return GetTargetPhraseCollection(phrase);
}

bool PhraseDictionary::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_filePath = value;
  } else if (key == "table-limit") {
    m_tableLimit = Scan<size_t>(value);
  } else {
    return DecodeFeature::SetParameter(key, value);
  }
  return true;
}

void PhraseDictionary::SetFeaturesToApply()
{
  // find out which feature function can be applied in this decode step
  const std::vector<FeatureFunction*> &allFeatures = FeatureFunction::GetFeatureFunctions();
  for (size_t i = 0; i < allFeatures.size(); ++i) {
    FeatureFunction *feature = allFeatures[i];
    if (feature->IsUseable(m_outputFactors)) {
      m_featuresToApply.push_back(feature);
    }
  }
}

}

