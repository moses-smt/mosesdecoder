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
#include "moses/InputFileStream.h"
#include "moses/StaticData.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/TranslationModel/CYKPlusParser/DotChartOnDisk.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerOnDisk.h"

using namespace std;


namespace Moses
{
PhraseDictionaryOnDisk::~PhraseDictionaryOnDisk()
{
}

bool PhraseDictionaryOnDisk::InitDictionary()
{
  PrintUserTime("Start loading binary SCFG phrase table. ");

  const StaticData &staticData = StaticData::Instance();
  m_languageModels = &staticData.GetLMList();
  m_wpProducer = staticData.GetWordPenaltyProducer();

  LoadTargetLookup();

  if (!m_dbWrapper.BeginLoad(m_filePath))
    return false;

  CHECK(m_dbWrapper.GetMisc("Version") == OnDiskPt::OnDiskWrapper::VERSION_NUM);
  CHECK(m_dbWrapper.GetMisc("NumSourceFactors") == m_input.size());
  CHECK(m_dbWrapper.GetMisc("NumTargetFactors") == m_output.size());
  CHECK(m_dbWrapper.GetMisc("NumScores") == m_numScoreComponents);

  return true;
}

// PhraseDictionary impl

//! find list of translations that can translates src. Only for phrase input
const TargetPhraseCollection *PhraseDictionaryOnDisk::GetTargetPhraseCollection(const Phrase& /* src */) const
{
  CHECK(false);
  return NULL;
}

void PhraseDictionaryOnDisk::LoadTargetLookup()
{
  // TODO
}

ChartRuleLookupManager *PhraseDictionaryOnDisk::CreateRuleLookupManager(
  const InputType &sentence,
  const ChartCellCollectionBase &cellCollection)
{
  return new ChartRuleLookupManagerOnDisk(sentence, cellCollection, *this,
                                          m_dbWrapper, m_languageModels,
                                          m_wpProducer, m_input,
                                          m_output, m_filePath);
}

}

