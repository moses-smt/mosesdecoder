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
  return true;
}

// PhraseDictionary impl

//! find list of translations that can translates src. Only for phrase input
const TargetPhraseCollection *PhraseDictionaryOnDisk::GetTargetPhraseCollection(const Phrase& /* src */) const
{
  CHECK(false);
  return NULL;
}

ChartRuleLookupManager *PhraseDictionaryOnDisk::CreateRuleLookupManager(
  const InputType &sentence,
  const ChartCellCollectionBase &cellCollection)
{
  return new ChartRuleLookupManagerOnDisk(sentence, cellCollection, *this,
                                          GetImplementation(),
                                          m_wpProducer, m_input,
                                          m_output, m_filePath);
}

OnDiskPt::OnDiskWrapper &PhraseDictionaryOnDisk::GetImplementation()
{
  OnDiskPt::OnDiskWrapper* dict;
  dict = m_implementation.get();
  CHECK(dict);
  return *dict;
}

const OnDiskPt::OnDiskWrapper &PhraseDictionaryOnDisk::GetImplementation() const
{
  OnDiskPt::OnDiskWrapper* dict;
  dict = m_implementation.get();
  CHECK(dict);
  return *dict;
}

void PhraseDictionaryOnDisk::InitializeForInput(InputType const& source)
{
  const StaticData &staticData = StaticData::Instance();

  OnDiskPt::OnDiskWrapper *obj = new OnDiskPt::OnDiskWrapper();
  if (!obj->BeginLoad(m_filePath))
    return;

  CHECK(obj->GetMisc("Version") == OnDiskPt::OnDiskWrapper::VERSION_NUM);
  CHECK(obj->GetMisc("NumSourceFactors") == m_input.size());
  CHECK(obj->GetMisc("NumTargetFactors") == m_output.size());
  CHECK(obj->GetMisc("NumScores") == m_numScoreComponents);

  m_implementation.reset(obj);

  return;
}

void PhraseDictionaryOnDisk::CleanUpAfterSentenceProcessing(InputType const& source)
{

}

}

