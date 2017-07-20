// $Id$

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

#include <fstream>
#include <string>
#include "GenerationDictionary.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "util/exception.hh"
#include "util/string_stream.hh"

using namespace std;

namespace Moses
{
std::vector<GenerationDictionary*> GenerationDictionary::s_staticColl;

GenerationDictionary::GenerationDictionary(const std::string &line)
  : DecodeFeature(line, true)
{
  s_staticColl.push_back(this);

  ReadParameters();
}

void GenerationDictionary::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  FactorCollection &factorCollection = FactorCollection::Instance();

  const size_t numFeatureValuesInConfig = this->GetNumScoreComponents();


  // data from file
  InputFileStream inFile(m_filePath);
  UTIL_THROW_IF2(!inFile.good(), "Couldn't read " << m_filePath);

  string line;
  size_t lineNum = 0;
  while(getline(inFile, line)) {
    ++lineNum;
    vector<string> token = Tokenize( line );

    // add each line in generation file into class
    Word *inputWord = new Word();  // deleted in destructor
    Word outputWord;

    // create word with certain factors filled out

    // inputs
    vector<string> factorString = Tokenize( token[0], "|" );
    for (size_t i = 0 ; i < GetInput().size() ; i++) {
      FactorType factorType = GetInput()[i];
      const Factor *factor = factorCollection.AddFactor( Output, factorType, factorString[i]);
      inputWord->SetFactor(factorType, factor);
    }

    factorString = Tokenize( token[1], "|" );
    for (size_t i = 0 ; i < GetOutput().size() ; i++) {
      FactorType factorType = GetOutput()[i];

      const Factor *factor = factorCollection.AddFactor( Output, factorType, factorString[i]);
      outputWord.SetFactor(factorType, factor);
    }

    size_t numFeaturesInFile = token.size() - 2;
    if (numFeaturesInFile < numFeatureValuesInConfig) {
      util::StringStream strme;
      strme << m_filePath << ":" << lineNum << ": expected " << numFeatureValuesInConfig
            << " feature values, but found " << numFeaturesInFile << "\n";
      throw strme.str();
    }
    std::vector<float> scores(numFeatureValuesInConfig, 0.0f);
    for (size_t i = 0; i < numFeatureValuesInConfig; i++)
      scores[i] = FloorScore(TransformScore(Scan<float>(token[2+i])));

    Collection::iterator iterWord = m_collection.find(inputWord);
    if (iterWord == m_collection.end()) {
      m_collection[inputWord][outputWord].Assign(this, scores);
    } else {
      // source word already in there. delete input word to avoid mem leak
      (iterWord->second)[outputWord].Assign(this, scores);
      delete inputWord;
    }
  }

  inFile.Close();
}

GenerationDictionary::~GenerationDictionary()
{
  Collection::const_iterator iter;
  for (iter = m_collection.begin() ; iter != m_collection.end() ; ++iter) {
    delete iter->first;
  }
}

const OutputWordCollection *GenerationDictionary::FindWord(const Word &word) const
{
  const OutputWordCollection *ret;

  Word wordInput;
  const std::vector<FactorType> &inputFactors = GetInput();
  for (size_t i = 0; i < inputFactors.size(); ++i) {
    FactorType factorType = inputFactors[i];
    wordInput[factorType] = word[factorType];
  }

  Collection::const_iterator iter = m_collection.find(&wordInput);
  if (iter == m_collection.end()) {
    // can't find source phrase
    ret = NULL;
  } else {
    ret = &iter->second;
  }
  return ret;
}

void GenerationDictionary::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "path") {
    m_filePath = value;
  } else {
    DecodeFeature::SetParameter(key, value);
  }
}

}

