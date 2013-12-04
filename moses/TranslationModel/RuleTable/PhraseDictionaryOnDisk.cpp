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
  : MyBase(line)
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
  UTIL_THROW_IF2(dict == NULL, "Dictionary object not yet created for this thread");
  return *dict;
}

const OnDiskPt::OnDiskWrapper &PhraseDictionaryOnDisk::GetImplementation() const
{
  OnDiskPt::OnDiskWrapper* dict;
  dict = m_implementation.get();
  UTIL_THROW_IF2(dict == NULL, "Dictionary object not yet created for this thread");
  return *dict;
}

void PhraseDictionaryOnDisk::InitializeForInput(InputType const& source)
{
  const StaticData &staticData = StaticData::Instance();

  ReduceCache();

  OnDiskPt::OnDiskWrapper *obj = new OnDiskPt::OnDiskWrapper();
  obj->BeginLoad(m_filePath);

  UTIL_THROW_IF2(obj->GetMisc("Version") != OnDiskPt::OnDiskWrapper::VERSION_NUM,
		  "On-disk phrase table is version " <<  obj->GetMisc("Version")
		  << ". It is not compatible with version " << OnDiskPt::OnDiskWrapper::VERSION_NUM);

  UTIL_THROW_IF2(obj->GetMisc("NumSourceFactors") != m_input.size(),
		  "On-disk phrase table has " <<  obj->GetMisc("NumSourceFactors") << " source factors."
		  		  << ". The ini file specified " << m_input.size() << " source factors");

  UTIL_THROW_IF2(obj->GetMisc("NumTargetFactors") != m_output.size(),
		  "On-disk phrase table has " <<  obj->GetMisc("NumTargetFactors") << " target factors."
		  		  << ". The ini file specified " << m_output.size() << " target factors");

  UTIL_THROW_IF2(obj->GetMisc("NumScores") != m_numScoreComponents,
		  "On-disk phrase table has " <<  obj->GetMisc("NumScores") << " scores."
		  		  << ". The ini file specified " << m_numScoreComponents << " scores");

  m_implementation.reset(obj);
}

void PhraseDictionaryOnDisk::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    GetTargetPhraseCollectionBatch(inputPath);
  }
}

void PhraseDictionaryOnDisk::GetTargetPhraseCollectionBatch(InputPath &inputPath) const
{
  OnDiskPt::OnDiskWrapper &wrapper = const_cast<OnDiskPt::OnDiskWrapper&>(GetImplementation());
  const Phrase &phrase = inputPath.GetPhrase();
  const InputPath *prevInputPath = inputPath.GetPrevPath();

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
        const TargetPhraseCollection *targetPhrases = GetTargetPhraseCollection(ptNode);
        inputPath.SetTargetPhrases(*this, targetPhrases, ptNode);
      } else {
        inputPath.SetTargetPhrases(*this, NULL, NULL);
      }

      delete lastWordOnDisk;
    }
  }
}

const TargetPhraseCollection *PhraseDictionaryOnDisk::GetTargetPhraseCollection(const OnDiskPt::PhraseNode *ptNode) const
{
  const TargetPhraseCollection *ret;

  if (m_maxCacheSize) {
    CacheColl &cache = GetCache();
    size_t hash = (size_t) ptNode->GetFilePos();

    std::map<size_t, std::pair<const TargetPhraseCollection*, clock_t> >::iterator iter;

    iter = cache.find(hash);

    if (iter == cache.end()) {
      // not in cache, need to look up from phrase table
      ret = GetTargetPhraseCollectionNonCache(ptNode);
      if (ret) {
        ret = new TargetPhraseCollection(*ret);
      }

      std::pair<const TargetPhraseCollection*, clock_t> value(ret, clock());
      cache[hash] = value;
    } else {
      // in cache. just use it
      std::pair<const TargetPhraseCollection*, clock_t> &value = iter->second;
      value.second = clock();

      ret = value.first;
    }
  } else {
    ret = GetTargetPhraseCollectionNonCache(ptNode);
  }

  return ret;
}

const TargetPhraseCollection *PhraseDictionaryOnDisk::GetTargetPhraseCollectionNonCache(const OnDiskPt::PhraseNode *ptNode) const
{
  OnDiskPt::OnDiskWrapper &wrapper = const_cast<OnDiskPt::OnDiskWrapper&>(GetImplementation());

  vector<float> weightT = StaticData::Instance().GetWeights(this);
  OnDiskPt::Vocab &vocab = wrapper.GetVocab();

  const OnDiskPt::TargetPhraseCollection *targetPhrasesOnDisk = ptNode->GetTargetPhraseCollection(m_tableLimit, wrapper);
  TargetPhraseCollection *targetPhrases
  = targetPhrasesOnDisk->ConvertToMoses(m_input, m_output, *this, weightT, vocab, false);

  delete targetPhrasesOnDisk;

  return targetPhrases;
}

} // namespace

