/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh
 
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

#include <vector>
#include "moses/InputFileStream.h"
#include "moses/Util.h"
#include "moses/StaticData.h"
#include "Trie.h"
#include "Loader.h"
#include "LoaderFactory.h"

using namespace std;

namespace Moses
{

RuleTableTrie::~RuleTableTrie()
{
}

bool RuleTableTrie::InitDictionary()
{

  std::auto_ptr<Moses::RuleTableLoader> loader =
      Moses::RuleTableLoaderFactory::Create(m_filePath);
  if (!loader.get()) {
    return false;
  }

  const StaticData &staticData = StaticData::Instance();

  vector<float> weight = staticData.GetWeights(this);
  const WordPenaltyProducer *wpProducer = staticData.GetWordPenaltyProducer();
  const LMList &languageModels = staticData.GetLMList();

  bool ret = loader->Load(m_input, m_output, m_filePath, weight, m_tableLimit,
                          languageModels, wpProducer, *this);
  return ret;
}


}  // namespace Moses
