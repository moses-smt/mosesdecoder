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

  TargetPhraseCollection::shared_ptr tpColl;
  bool ok;
  vector<uint64_t> probingSource = ConvertToProbingSourcePhrase(sourcePhrase, ok);
  if (!ok) {
    // source phrase contains a word unknown in the pt.
    // We know immediately there's no translation for it
    return tpColl;
  }

  std::pair<bool, std::vector<target_text> > query_result;

  //Actual lookup
  query_result = m_engine->query(probingSource);

  if (query_result.first) {
    //m_engine->printTargetInfo(query_result.second);
    tpColl.reset(new TargetPhraseCollection());

    const std::vector<target_text> &probingTargetPhrases = query_result.second;
    for (size_t i = 0; i < probingTargetPhrases.size(); ++i) {
      const target_text &probingTargetPhrase = probingTargetPhrases[i];
      TargetPhrase *tp = CreateTargetPhrase(sourcePhrase, probingTargetPhrase);

      tpColl->Add(tp);
    }

    tpColl->Prune(true, m_tableLimit);
  }

  return tpColl;
}

TargetPhrase *ProbingPT::CreateTargetPhrase(const Phrase &sourcePhrase, const target_text &probingTargetPhrase) const
{
  const std::vector<unsigned int> &probingPhrase = probingTargetPhrase.target_phrase;
  size_t size = probingPhrase.size();

  TargetPhrase *tp = new TargetPhrase(this);

  // words
  for (size_t i = 0; i < size; ++i) {
    uint64_t probingId = probingPhrase[i];
    const Factor *factor = GetTargetFactor(probingId);
    assert(factor);

    Word &word = tp->AddWord();
    word.SetFactor(m_output[0], factor);
  }

  // score for this phrase table
  vector<float> scores = probingTargetPhrase.prob;
  std::transform(scores.begin(), scores.end(), scores.begin(),TransformScore);
  tp->GetScoreBreakdown().PlusEquals(this, scores);

  // alignment
  /*
  const std::vector<unsigned char> &alignments = probingTargetPhrase.word_all1;

  AlignmentInfo &aligns = tp->GetAlignTerm();
  for (size_t i = 0; i < alignS.size(); i += 2 ) {
    aligns.Add((size_t) alignments[i], (size_t) alignments[i+1]);
  }
  */

  // score of all other ff when this rule is being loaded
  tp->EvaluateInIsolation(sourcePhrase, GetFeaturesToApply());
  return tp;
}

const Factor *ProbingPT::GetTargetFactor(uint64_t probingId) const
{
  TargetVocabMap::right_map::const_iterator iter;
  iter = m_vocabMap.right.find(probingId);
  if (iter != m_vocabMap.right.end()) {
    return iter->second;
  } else {
    // not in mapping. Must be UNK
    return NULL;
  }
}

uint64_t ProbingPT::GetSourceProbingId(const Factor *factor) const
{
  SourceVocabMap::left_map::const_iterator iter;
  iter = m_sourceVocabMap.left.find(factor);
  if (iter != m_sourceVocabMap.left.end()) {
    return iter->second;
  } else {
    // not in mapping. Must be UNK
    return m_unkId;
  }
}

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
