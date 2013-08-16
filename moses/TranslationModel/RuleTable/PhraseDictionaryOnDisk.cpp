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
#include "moses/InputPath.h"
#include "moses/TranslationModel/CYKPlusParser/DotChartOnDisk.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerOnDisk.h"

#include "OnDiskPt/OnDiskWrapper.h"
#include "OnDiskPt/Word.h"

using namespace std;


namespace Moses
{
PhraseDictionaryOnDisk::PhraseDictionaryOnDisk(const std::string &line)
  : MyBase("PhraseDictionaryOnDisk", line)
{
  ReadParameters();
}

PhraseDictionaryOnDisk::~PhraseDictionaryOnDisk()
{
}

void PhraseDictionaryOnDisk::Load()
{
  SetFeaturesToApply();
}

ChartRuleLookupManager *PhraseDictionaryOnDisk::CreateRuleLookupManager(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellCollection)
{
  return new ChartRuleLookupManagerOnDisk(parser, cellCollection, *this,
                                          GetImplementation(),
                                          m_input,
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
}

void PhraseDictionaryOnDisk::GetTargetPhraseCollectionBatch(const InputPathList &phraseDictionaryQueue) const
{
  OnDiskPt::OnDiskWrapper &wrapper = const_cast<OnDiskPt::OnDiskWrapper&>(GetImplementation());

  InputPathList::const_iterator iter;
  for (iter = phraseDictionaryQueue.begin(); iter != phraseDictionaryQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const Phrase &phrase = inputPath.GetPhrase();
    const InputPath *prevInputPath = inputPath.GetPrevNode();

    const OnDiskPt::PhraseNode *prevPtNode = NULL;

    if (prevInputPath) {
      prevPtNode = static_cast<const OnDiskPt::PhraseNode*>(prevInputPath->GetPtNode(*this));
    } else {
      // Starting subphrase.
      assert(phrase.GetSize() == 1);
      prevPtNode = &wrapper.GetRootSourceNode();
    }

    if (prevPtNode) {
      Word lastWord = phrase.GetWord(phrase.GetSize() - 1);
      lastWord.OnlyTheseFactors(m_inputFactors);
      OnDiskPt::Word *lastWordOnDisk = wrapper.ConvertFromMoses(m_input, lastWord);

      if (lastWordOnDisk == NULL) {
        // OOV according to this phrase table. Not possible to extend
        inputPath.SetTargetPhrases(*this, NULL, NULL);
      } else {
        const OnDiskPt::PhraseNode *ptNode = prevPtNode->GetChild(*lastWordOnDisk, wrapper);
        if (ptNode) {
          vector<float> weightT = StaticData::Instance().GetWeights(this);
          OnDiskPt::Vocab &vocab = wrapper.GetVocab();

          const OnDiskPt::TargetPhraseCollection *targetPhrasesOnDisk = ptNode->GetTargetPhraseCollection(m_tableLimit, wrapper);
          TargetPhraseCollection *targetPhrases
          = targetPhrasesOnDisk->ConvertToMoses(m_input, m_output, *this, weightT, vocab, false);

          inputPath.SetTargetPhrases(*this, targetPhrases, ptNode);

          delete targetPhrasesOnDisk;

        } else {
          inputPath.SetTargetPhrases(*this, NULL, NULL);
        }

        delete lastWordOnDisk;
      }
    }
  }
}

}

