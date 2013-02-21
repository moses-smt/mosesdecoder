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

#include "moses/InputFileStream.h"
#include "moses/Util.h"
#include "Trie.h"
#include "Loader.h"
#include "LoaderFactory.h"

namespace Moses
{

RuleTableTrie::~RuleTableTrie()
{
  CleanUp();
}

bool RuleTableTrie::Load(const std::vector<FactorType> &input,
                         const std::vector<FactorType> &output,
                         const std::string &filePath,
                         const std::vector<float> &weight,
                         size_t tableLimit,
                         const LMList &languageModels,
                         const WordPenaltyProducer* wpProducer)
{
  m_filePath = filePath;
  m_tableLimit = tableLimit;

  std::auto_ptr<Moses::RuleTableLoader> loader =
      Moses::RuleTableLoaderFactory::Create(filePath);
  if (!loader.get())
  {
    return false;
  }
  bool ret = loader->Load(input, output, filePath, weight, tableLimit,
                          languageModels, wpProducer, *this);
  return ret;
}

void RuleTableTrie::InitializeForInput(const InputType& /* input */)
{
  // Nothing to do: sentence-specific state is stored in ChartRuleLookupManager
}

void RuleTableTrie::CleanUp()
{
  // Nothing to do: sentence-specific state is stored in ChartRuleLookupManager
}

}  // namespace Moses
