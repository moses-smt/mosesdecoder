// vim:tabstop=2
#include "ProbingPT.h"
#include "moses/StaticData.h"
#include "moses/FactorCollection.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/InputFileStream.h"
#include "probingpt/querying.h"
#include "probingpt/probing_hash_utils.h"

using namespace std;

namespace Moses
{
ProbingPT::ProbingPT(const std::string &line)
  : PhraseDictionary(line,true)
  ,m_engine(NULL)
  ,load_method(util::POPULATE_OR_READ)
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

  m_engine = new probingpt::QueryEngine(m_filePath.c_str(), load_method);

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

void ProbingPT::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "load") {
    if (value == "lazy") {
      load_method = util::LAZY;
    } else if (value == "populate_or_lazy") {
      load_method = util::POPULATE_OR_LAZY;
    } else if (value == "populate_or_read" || value == "populate") {
      load_method = util::POPULATE_OR_READ;
    } else if (value == "read") {
      load_method = util::READ;
    } else if (value == "parallel_read") {
      load_method = util::PARALLEL_READ;
    } else {
      UTIL_THROW2("load method not supported" << value);
    }
  } else {
    PhraseDictionary::SetParameter(key, value);
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
  } else {
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
    } else {
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
  TargetPhraseCollection *tps = NULL;

  //Actual lookup
  std::pair<bool, uint64_t> query_result; // 1st=found, 2nd=target file offset
  query_result = m_engine->query(key);
  //cerr << "key2=" << query_result.second << endl;

  if (query_result.first) {
    const char *offset = data + query_result.second;
    uint64_t *numTP = (uint64_t*) offset;

    tps = new TargetPhraseCollection();

    offset += sizeof(uint64_t);
    for (size_t i = 0; i < *numTP; ++i) {
      TargetPhrase *tp = CreateTargetPhrase(offset);
      assert(tp);
      tp->EvaluateInIsolation(sourcePhrase, GetFeaturesToApply());

      tps->Add(tp);

    }

    tps->Prune(true, m_tableLimit);
    //cerr << *tps << endl;
  }

  return tps;

}

TargetPhrase *ProbingPT::CreateTargetPhrase(
  const char *&offset) const
{
  probingpt::TargetPhraseInfo *tpInfo = (probingpt::TargetPhraseInfo*) offset;
  size_t numRealWords = tpInfo->numWords / m_output.size();

  TargetPhrase *tp = new TargetPhrase(this);

  offset += sizeof(probingpt::TargetPhraseInfo);

  // scores
  float *scores = (float*) offset;

  size_t totalNumScores = m_engine->num_scores + m_engine->num_lex_scores;

  if (m_engine->logProb) {
    // set pt score for rule
    tp->GetScoreBreakdown().PlusEquals(this, scores);

    // save scores for other FF, eg. lex RO. Just give the offset
    /*
    if (m_engine->num_lex_scores) {
      tp->scoreProperties = scores + m_engine->num_scores;
    }
    */
  } else {
    // log score 1st
    float logScores[totalNumScores];
    for (size_t i = 0; i < totalNumScores; ++i) {
      logScores[i] = FloorScore(TransformScore(scores[i]));
    }

    // set pt score for rule
    tp->GetScoreBreakdown().PlusEquals(this, logScores);

    // save scores for other FF, eg. lex RO.
    /*
    tp->scoreProperties = pool.Allocate<SCORE>(m_engine->num_lex_scores);
    for (size_t i = 0; i < m_engine->num_lex_scores; ++i) {
      tp->scoreProperties[i] = logScores[i + m_engine->num_scores];
    }
    */
  }

  offset += sizeof(float) * totalNumScores;

  // words
  for (size_t targetPos = 0; targetPos < numRealWords; ++targetPos) {
    Word &word = tp->AddWord();
    for (size_t i = 0; i < m_output.size(); ++i) {
      FactorType factorType = m_output[i];

      uint32_t *probingId = (uint32_t*) offset;

      const Factor *factor = GetTargetFactor(*probingId);
      assert(factor);

      word[factorType] = factor;

      offset += sizeof(uint32_t);
    }
  }

  // align
  uint32_t alignTerm = tpInfo->alignTerm;
  //cerr << "alignTerm=" << alignTerm << endl;
  UTIL_THROW_IF2(alignTerm >= m_aligns.size(), "Unknown alignInd");
  tp->SetAlignTerm(m_aligns[alignTerm]);

  // properties TODO

  return tp;
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
