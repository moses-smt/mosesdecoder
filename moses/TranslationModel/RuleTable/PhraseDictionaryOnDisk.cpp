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
#include "moses/TranslationTask.h"

#include "OnDiskPt/OnDiskWrapper.h"
#include "OnDiskPt/Word.h"

#include "util/tokenize_piece.hh"

using namespace std;


namespace Moses
{
PhraseDictionaryOnDisk::PhraseDictionaryOnDisk(const std::string &line)
  : MyBase(line, true)
  , m_maxSpanDefault(NOT_FOUND)
  , m_maxSpanLabelled(NOT_FOUND)
{
  ReadParameters();
}

PhraseDictionaryOnDisk::~PhraseDictionaryOnDisk()
{
}

void PhraseDictionaryOnDisk::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();
}

ChartRuleLookupManager *PhraseDictionaryOnDisk::CreateRuleLookupManager(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellCollection,
  std::size_t /*maxChartSpan*/)
{
  return new ChartRuleLookupManagerOnDisk(parser, cellCollection, *this,
                                          GetImplementation(),
                                          m_input,
                                          m_output);
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

void PhraseDictionaryOnDisk::InitializeForInput(ttasksptr const& ttask)
{
  InputType const& source = *ttask->GetSource();
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

  // delete nodes that's been saved
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const OnDiskPt::PhraseNode *ptNode = static_cast<const OnDiskPt::PhraseNode*>(inputPath.GetPtNode(*this));
    delete ptNode;
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

  // backoff
  if (!SatisfyBackoff(inputPath)) {
    return;
  }

  if (prevPtNode) {
    Word lastWord = phrase.GetWord(phrase.GetSize() - 1);
    lastWord.OnlyTheseFactors(m_inputFactors);
    OnDiskPt::Word *lastWordOnDisk = ConvertFromMoses(wrapper, m_input, lastWord);

    TargetPhraseCollection::shared_ptr tpc;
    if (lastWordOnDisk == NULL) {
      // OOV according to this phrase table. Not possible to extend
      inputPath.SetTargetPhrases(*this, tpc, NULL);
    } else {
      OnDiskPt::PhraseNode const* ptNode;
      ptNode = prevPtNode->GetChild(*lastWordOnDisk, wrapper);
      if (ptNode) tpc = GetTargetPhraseCollection(ptNode);
      inputPath.SetTargetPhrases(*this, tpc, ptNode);

      delete lastWordOnDisk;
    }
  }
}

TargetPhraseCollection::shared_ptr
PhraseDictionaryOnDisk::
GetTargetPhraseCollection(const OnDiskPt::PhraseNode *ptNode) const
{
  TargetPhraseCollection::shared_ptr ret;

  CacheColl &cache = GetCache();
  size_t hash = (size_t) ptNode->GetFilePos();

  CacheColl::iterator iter;

  iter = cache.find(hash);

  if (iter == cache.end()) {
    // not in cache, need to look up from phrase table
    ret = GetTargetPhraseCollectionNonCache(ptNode);

    std::pair<TargetPhraseCollection::shared_ptr , clock_t> value(ret, clock());
    cache[hash] = value;
  } else {
    // in cache. just use it
    iter->second.second = clock();
    ret = iter->second.first;
  }

  return ret;
}

TargetPhraseCollection::shared_ptr
PhraseDictionaryOnDisk::
GetTargetPhraseCollectionNonCache(const OnDiskPt::PhraseNode *ptNode) const
{
  OnDiskPt::OnDiskWrapper& wrapper
  = const_cast<OnDiskPt::OnDiskWrapper&>(GetImplementation());

  vector<float> weightT = StaticData::Instance().GetWeights(this);
  OnDiskPt::Vocab &vocab = wrapper.GetVocab();

  OnDiskPt::TargetPhraseCollection::shared_ptr targetPhrasesOnDisk
  = ptNode->GetTargetPhraseCollection(m_tableLimit, wrapper);
  TargetPhraseCollection::shared_ptr targetPhrases
  = ConvertToMoses(targetPhrasesOnDisk, m_input, m_output, *this,
                   weightT, vocab, false);

  // delete targetPhrasesOnDisk;

  return targetPhrases;
}

Moses::TargetPhraseCollection::shared_ptr
PhraseDictionaryOnDisk::ConvertToMoses(
  const OnDiskPt::TargetPhraseCollection::shared_ptr targetPhrasesOnDisk
  , const std::vector<Moses::FactorType> &inputFactors
  , const std::vector<Moses::FactorType> &outputFactors
  , const Moses::PhraseDictionary &phraseDict
  , const std::vector<float> &weightT
  , OnDiskPt::Vocab &vocab
  , bool isSyntax) const
{
  Moses::TargetPhraseCollection::shared_ptr ret;
  ret.reset(new Moses::TargetPhraseCollection);

  for (size_t i = 0; i < targetPhrasesOnDisk->GetSize(); ++i) {
    const OnDiskPt::TargetPhrase &tp = targetPhrasesOnDisk->GetTargetPhrase(i);
    Moses::TargetPhrase *mosesPhrase
    = ConvertToMoses(tp, inputFactors, outputFactors, vocab,
                     phraseDict, weightT, isSyntax);

    /*
    // debugging output
    stringstream strme;
    strme << filePath << " " << *mosesPhrase;
    mosesPhrase->SetDebugOutput(strme.str());
    */

    ret->Add(mosesPhrase);
  }

  ret->Sort(true, phraseDict.GetTableLimit());

  return ret;
}

Moses::TargetPhrase *PhraseDictionaryOnDisk::ConvertToMoses(const OnDiskPt::TargetPhrase &targetPhraseOnDisk
    , const std::vector<Moses::FactorType> &inputFactors
    , const std::vector<Moses::FactorType> &outputFactors
    , const OnDiskPt::Vocab &vocab
    , const Moses::PhraseDictionary &phraseDict
    , const std::vector<float> &weightT
    , bool isSyntax) const
{
  Moses::TargetPhrase *ret = new Moses::TargetPhrase(&phraseDict);

  // words
  size_t phraseSize = targetPhraseOnDisk.GetSize();
  UTIL_THROW_IF2(phraseSize == 0, "Target phrase cannot be empty"); // last word is lhs
  if (isSyntax) {
    --phraseSize;
  }

  for (size_t pos = 0; pos < phraseSize; ++pos) {
    const OnDiskPt::Word &wordOnDisk = targetPhraseOnDisk.GetWord(pos);
    ConvertToMoses(wordOnDisk, outputFactors, vocab, ret->AddWord());
  }

  // alignments
  // int index = 0;
  Moses::AlignmentInfo::CollType alignTerm, alignNonTerm;
  std::set<std::pair<size_t, size_t> > alignmentInfo;
  const OnDiskPt::PhrasePtr sp = targetPhraseOnDisk.GetSourcePhrase();
  for (size_t ind = 0; ind < targetPhraseOnDisk.GetAlign().size(); ++ind) {
    const std::pair<size_t, size_t> &entry = targetPhraseOnDisk.GetAlign()[ind];
    alignmentInfo.insert(entry);
    size_t sourcePos = entry.first;
    size_t targetPos = entry.second;

    if (targetPhraseOnDisk.GetWord(targetPos).IsNonTerminal()) {
      alignNonTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    } else {
      alignTerm.insert(std::pair<size_t,size_t>(sourcePos, targetPos));
    }

  }
  ret->SetAlignTerm(alignTerm);
  ret->SetAlignNonTerm(alignNonTerm);

  if (isSyntax) {
    Moses::Word *lhsTarget = new Moses::Word(true);
    const OnDiskPt::Word &lhsOnDisk = targetPhraseOnDisk.GetWord(targetPhraseOnDisk.GetSize() - 1);
    ConvertToMoses(lhsOnDisk, outputFactors, vocab, *lhsTarget);
    ret->SetTargetLHS(lhsTarget);
  }

  // set source phrase
  Moses::Phrase mosesSP(Moses::Input);
  for (size_t pos = 0; pos < sp->GetSize(); ++pos) {
    ConvertToMoses(sp->GetWord(pos), inputFactors, vocab, mosesSP.AddWord());
  }

  // scores
  ret->GetScoreBreakdown().Assign(&phraseDict, targetPhraseOnDisk.GetScores());

  // sparse features
  ret->GetScoreBreakdown().Assign(&phraseDict, targetPhraseOnDisk.GetSparseFeatures());

  // property
  ret->SetProperties(targetPhraseOnDisk.GetProperty());

  ret->EvaluateInIsolation(mosesSP, phraseDict.GetFeaturesToApply());

  return ret;
}

void PhraseDictionaryOnDisk::ConvertToMoses(
  const OnDiskPt::Word &wordOnDisk,
  const std::vector<Moses::FactorType> &outputFactorsVec,
  const OnDiskPt::Vocab &vocab,
  Moses::Word &overwrite) const
{
  Moses::FactorCollection &factorColl = Moses::FactorCollection::Instance();
  overwrite = Moses::Word(wordOnDisk.IsNonTerminal());

  if (wordOnDisk.IsNonTerminal()) {
    const std::string &tok = vocab.GetString(wordOnDisk.GetVocabId());
    overwrite.SetFactor(0, factorColl.AddFactor(tok, wordOnDisk.IsNonTerminal()));
  } else {
    // TODO: this conversion should have been done at load time.
    util::TokenIter<util::SingleCharacter> tok(vocab.GetString(wordOnDisk.GetVocabId()), '|');

    for (std::vector<Moses::FactorType>::const_iterator t = outputFactorsVec.begin(); t != outputFactorsVec.end(); ++t, ++tok) {
      UTIL_THROW_IF2(!tok, "Too few factors in \"" << vocab.GetString(wordOnDisk.GetVocabId()) << "\"; was expecting " << outputFactorsVec.size());
      overwrite.SetFactor(*t, factorColl.AddFactor(*tok, wordOnDisk.IsNonTerminal()));
    }
    UTIL_THROW_IF2(tok, "Too many factors in \"" << vocab.GetString(wordOnDisk.GetVocabId()) << "\"; was expecting " << outputFactorsVec.size());
  }
}

OnDiskPt::Word *PhraseDictionaryOnDisk::ConvertFromMoses(OnDiskPt::OnDiskWrapper &wrapper, const std::vector<Moses::FactorType> &factorsVec
    , const Moses::Word &origWord) const
{
  bool isNonTerminal = origWord.IsNonTerminal();
  OnDiskPt::Word *newWord = new OnDiskPt::Word(isNonTerminal);

  util::StringStream strme;

  size_t factorType = factorsVec[0];
  const Moses::Factor *factor = origWord.GetFactor(factorType);
  UTIL_THROW_IF2(factor == NULL, "Expecting factor " << factorType);
  strme << factor->GetString();

  for (size_t ind = 1 ; ind < factorsVec.size() ; ++ind) {
    size_t factorType = factorsVec[ind];
    const Moses::Factor *factor = origWord.GetFactor(factorType);
    if (factor == NULL) {
      // can have less factors than factorType.size()
      break;
    }
    UTIL_THROW_IF2(factor == NULL,
                   "Expecting factor " << factorType << " at position " << ind);
    strme << "|" << factor->GetString();
  } // for (size_t factorType

  bool found;
  uint64_t vocabId = wrapper.GetVocab().GetVocabId(strme.str(), found);
  if (!found) {
    // factor not in phrase table -> phrse definately not in. exit
    delete newWord;
    return NULL;
  } else {
    newWord->SetVocabId(vocabId);
    return newWord;
  }

}

void PhraseDictionaryOnDisk::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "max-span-default") {
    m_maxSpanDefault = Scan<size_t>(value);
  } else if (key == "max-span-labelled") {
    m_maxSpanLabelled = Scan<size_t>(value);
  } else {
    PhraseDictionary::SetParameter(key, value);
  }
}


} // namespace

