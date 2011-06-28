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
#include <fstream>

#include "FactorCollection.h"
#include "Util.h"

#include "Gain.h"

using namespace Moses;
using namespace std;

namespace Josiah {


void TextToTranslation(const string& text, Translation& words) {
  vector<string> tokens = Tokenize(text);
  words.clear();
  FactorCollection& factorCollection = FactorCollection::Instance();
  for (size_t i = 0; i < tokens.size(); ++i) {
    const Factor* factor = factorCollection.AddFactor(Input, 0, tokens[i]);
    words.push_back(factor);
  }
}

void Gain::LoadReferences(const vector<string>& refFiles,
     const string& sourceFile) {
  assert(refFiles.size());
  vector<boost::shared_ptr<ifstream> > refIns(refFiles.size());
  for (size_t i = 0; i < refFiles.size(); ++i) {
    refIns[i].reset(new ifstream());
    refIns[i]->open(refFiles[i].c_str());
    assert(refIns[i]->good());
  }
  ifstream srcIn(sourceFile.c_str());
  assert(srcIn);

  size_t count = 0;
  while(srcIn.good()) {
    string line;
    getline(srcIn,line);
    if (line.empty()) continue;
    Translation source;
    TextToTranslation(line,source);
    vector<Translation> refs(refFiles.size());;
    for (size_t i = 0; i < refFiles.size(); ++i) {
      getline(*refIns[i],line);
      assert(refIns[i]->good());
      TextToTranslation(line,refs[i]);
    }
    AddReferences(refs,source);
    ++count;
  }
  //check we were at the end of all the references
  for (size_t i = 0; i < refFiles.size(); ++i) {
    string line;
    getline(*refIns[i],line);
    assert(line.empty());
  }
  VERBOSE(1, "Loaded " << count << " references" << endl);
}

GainFunctionHandle Gain::GetGainFunction(size_t sentenceId) {
  vector<size_t> sentenceIds;
  sentenceIds.push_back(sentenceId);
  return GetGainFunction(sentenceIds);
}


float GainFunction::Evaluate(const Translation& hypothesis) const {
  vector<Translation> hyps;
  hyps.push_back(hypothesis);
  return Evaluate(hyps);
}



}
