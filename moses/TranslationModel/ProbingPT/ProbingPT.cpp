// vim:tabstop=2
#include "ProbingPT.h"
#include "moses/StaticData.h"
#include "moses/FactorCollection.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/InputFileStream.h"
#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerSkeleton.h"
#include "quering.hh"

using namespace std;

namespace Moses
{
ProbingPT::ProbingPT(const std::string &line)
  : PhraseDictionary(line,true)
  ,m_engine(NULL)
{
  ReadParameters();

  assert(m_input.size() == 1);
  assert(m_output.size() == 1);
}

ProbingPT::~ProbingPT()
{
  delete m_engine;
}

void ProbingPT::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
  SetFeaturesToApply();

  m_engine = new QueryEngine(m_filePath.c_str());

  m_unkId = 456456546456;

  FactorCollection &vocab = FactorCollection::Instance();

  // source vocab
  const std::map<uint64_t, std::string> &sourceVocab =
      m_engine->getSourceVocab();
  std::map<uint64_t, std::string>::const_iterator iterSource;
  for (iterSource = sourceVocab.begin(); iterSource != sourceVocab.end();
      ++iterSource) {
    string wordStr = iterSource->second;
    //cerr << "wordStr=" << wordStr << endl;

    const Factor *factor = vocab.AddFactor(wordStr);

    uint64_t probingId = iterSource->first;
    size_t factorId = factor->GetId();

    if (factorId >= m_sourceVocab.size()) {
      m_sourceVocab.resize(factorId + 1, m_unkId);
    }
    m_sourceVocab[factorId] = probingId;
  }

  // target vocab
  InputFileStream targetVocabStrme(m_filePath + "/TargetVocab.dat");
  string line;
  while (getline(targetVocabStrme, line)) {
    vector<string> toks = Tokenize(line, "\t");
    UTIL_THROW_IF2(toks.size() != 2, string("Incorrect format:") + line + "\n");

    //cerr << "wordStr=" << toks[0] << endl;

    const Factor *factor = vocab.AddFactor(toks[0]);
    uint32_t probingId = Scan<uint32_t>(toks[1]);

    if (probingId >= m_targetVocab.size()) {
      m_targetVocab.resize(probingId + 1);
    }

    m_targetVocab[probingId] = factor;
  }

  // alignments
  CreateAlignmentMap(m_filePath + "/Alignments.dat");

  // memory mapped file to tps
  string filePath = m_filePath + "/TargetColl.dat";
  file.open(filePath.c_str());
  if (!file.is_open()) {
    throw "Couldn't open file ";
  }

  data = file.data();
  //size_t size = file.size();

  // cache
  //CreateCache(system);

}

void ProbingPT::CreateAlignmentMap(const std::string path)
{
  const std::vector< std::vector<unsigned char> > &probingAlignColl = m_engine->getAlignments();
  m_aligns.resize(probingAlignColl.size(), NULL);

  for (size_t i = 0; i < probingAlignColl.size(); ++i) {
    AlignmentInfo::CollType aligns;

    const std::vector<unsigned char> &probingAligns = probingAlignColl[i];
    for (size_t j = 0; j < probingAligns.size(); j += 2) {
      size_t startPos = probingAligns[j];
      size_t endPos = probingAligns[j+1];
      //cerr << "startPos=" << startPos << " " << endPos << endl;
      aligns.insert(std::pair<size_t,size_t>(startPos, endPos));
    }

    const AlignmentInfo *align = AlignmentInfoCollection::Instance().Add(aligns);
    m_aligns[i] = align;
    //cerr << "align=" << align->Debug(system) << endl;
  }
}

void ProbingPT::InitializeForInput(ttasksptr const& ttask)
{

}

void ProbingPT::GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
    const Phrase &sourcePhrase = inputPath.GetPhrase();

    if (sourcePhrase.GetSize() > m_options->search.max_phrase_length) {
      continue;
    }

    TargetPhraseCollection::shared_ptr tpColl = CreateTargetPhrase(sourcePhrase);
    inputPath.SetTargetPhrases(*this, tpColl, NULL);
  }
}

std::vector<uint64_t> ProbingPT::ConvertToProbingSourcePhrase(const Phrase &sourcePhrase, bool &ok) const
{
  size_t size = sourcePhrase.GetSize();
  std::vector<uint64_t> ret(size);
  for (size_t i = 0; i < size; ++i) {
    const Factor *factor = sourcePhrase.GetFactor(i, m_input[0]);
    uint64_t probingId = GetSourceProbingId(factor);
    if (probingId == m_unkId) {
      ok = false;
      return ret;
    } else {
      ret[i] = probingId;
    }
  }

  ok = true;
  return ret;
}

TargetPhraseCollection::shared_ptr ProbingPT::CreateTargetPhrase(const Phrase &sourcePhrase) const
{
  // create a target phrase from the 1st word of the source, prefix with 'ProbingPT:'
  assert(sourcePhrase.GetSize());

  std::pair<bool, uint64_t> keyStruct = GetKey(sourcePhrase);
  if (!keyStruct.first) {
    return TargetPhraseCollection::shared_ptr();
  }

  // check in cache
  CachePb::const_iterator iter = m_cachePb.find(keyStruct.second);
  if (iter != m_cachePb.end()) {
    //cerr << "FOUND IN CACHE " << keyStruct.second << " " << sourcePhrase.Debug(mgr.system) << endl;
    TargetPhraseCollection *tps = iter->second;
    return TargetPhraseCollection::shared_ptr(tps);
  }

  // query pt
  TargetPhraseCollection *tps = CreateTargetPhrases(sourcePhrase,
      keyStruct.second);
  return TargetPhraseCollection::shared_ptr(tps);
}

std::pair<bool, uint64_t> ProbingPT::GetKey(const Phrase &sourcePhrase) const
{
  std::pair<bool, uint64_t> ret;

  // create a target phrase from the 1st word of the source, prefix with 'ProbingPT:'
  size_t sourceSize = sourcePhrase.GetSize();
  assert(sourceSize);

  uint64_t probingSource[sourceSize];
  GetSourceProbingIds(sourcePhrase, ret.first, probingSource);
  if (!ret.first) {
    // source phrase contains a word unknown in the pt.
    // We know immediately there's no translation for it
  }
  else {
    ret.second = m_engine->getKey(probingSource, sourceSize);
  }

  return ret;

}

void ProbingPT::GetSourceProbingIds(const Phrase &sourcePhrase,
    bool &ok, uint64_t probingSource[]) const
{

  size_t size = sourcePhrase.GetSize();
  for (size_t i = 0; i < size; ++i) {
    const Word &word = sourcePhrase.GetWord(i);
    uint64_t probingId = GetSourceProbingId(word);
    if (probingId == m_unkId) {
      ok = false;
      return;
    }
    else {
      probingSource[i] = probingId;
    }
  }

  ok = true;
}

uint64_t ProbingPT::GetSourceProbingId(const Word &word) const
{
  uint64_t ret = 0;

  for (size_t i = 0; i < m_input.size(); ++i) {
    FactorType factorType = m_input[i];
    const Factor *factor = word[factorType];

    size_t factorId = factor->GetId();
    if (factorId >= m_sourceVocab.size()) {
    return m_unkId;
    }
    ret += m_sourceVocab[factorId];
  }

  return ret;
}

TargetPhraseCollection *ProbingPT::CreateTargetPhrases(
    const Phrase &sourcePhrase, uint64_t key) const
{

}

//////////////////////////////////////////////////////////////////


ChartRuleLookupManager *ProbingPT::CreateRuleLookupManager(
  const ChartParser &,
  const ChartCellCollectionBase &,
  std::size_t)
{
  abort();
  return NULL;
}

TO_STRING_BODY(ProbingPT);

// friend
ostream& operator<<(ostream& out, const ProbingPT& phraseDict)
{
  return out;
}

}
