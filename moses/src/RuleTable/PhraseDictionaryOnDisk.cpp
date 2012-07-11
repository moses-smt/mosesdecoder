  // vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include "PhraseDictionaryOnDisk.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "TargetPhraseCollection.h"
#include "CYKPlusParser/DotChartOnDisk.h"
#include "CYKPlusParser/ChartRuleLookupManagerOnDisk.h"

using namespace std;


namespace Moses
{
PhraseDictionaryOnDisk::~PhraseDictionaryOnDisk()
{
  CleanUp();
}

bool PhraseDictionaryOnDisk::Load(const std::vector<FactorType> &input
                                  , const std::vector<FactorType> &output
                                  , const std::string &filePath
                                  , const std::vector<float> &weight
                                  , size_t tableLimit
                                  , const LMList& languageModels
                                  , const WordPenaltyProducer* wpProducer)
{
  m_languageModels = &(languageModels);
  m_wpProducer = wpProducer;
  m_filePath = filePath;
  m_tableLimit = tableLimit;
  m_inputFactorsVec		= input;
  m_outputFactorsVec	= output;

  m_weight = weight;

  LoadTargetLookup();

  if (!m_dbWrapper.BeginLoad(filePath))
    return false;

  CHECK(m_dbWrapper.GetMisc("Version") == 4);
  CHECK(m_dbWrapper.GetMisc("NumSourceFactors") == input.size());
  CHECK(m_dbWrapper.GetMisc("NumTargetFactors") == output.size());
  CHECK(m_dbWrapper.GetMisc("NumScores") == weight.size());

  return true;
}

//! find list of translations that can translates src. Only for phrase input
const TargetPhraseCollection *PhraseDictionaryOnDisk::GetTargetPhraseCollection(const Phrase& /* src */) const
{
  CHECK(false);
  return NULL;
}

void PhraseDictionaryOnDisk::InitializeForInput(const InputType& /* input */)
{
  // Nothing to do: sentence-specific state is stored in ChartRuleLookupManager
}

void PhraseDictionaryOnDisk::CleanUp()
{
  // Nothing to do: sentence-specific state is stored in ChartRuleLookupManager
}

void PhraseDictionaryOnDisk::LoadTargetLookup()
{
  // TODO
}

ChartRuleLookupManager *PhraseDictionaryOnDisk::CreateRuleLookupManager(
  const InputType &sentence,
  const ChartCellCollection &cellCollection)
{
  return new ChartRuleLookupManagerOnDisk(sentence, cellCollection, *this,
                                          m_dbWrapper, m_languageModels,
                                          m_wpProducer, m_inputFactorsVec,
                                          m_outputFactorsVec, m_weight,
                                          m_filePath);
}

}

