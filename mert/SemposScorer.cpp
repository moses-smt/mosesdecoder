#include "SemposScorer.h"

#include <algorithm>
#include <vector>
#include <stdexcept>

#include "Util.h"

using namespace std;

SemposScorer::SemposScorer(const string& config)
  : StatisticsBasedScorer("SEMPOS", config),
    m_ovr(SemposOverlappingFactory::GetOverlapping(getConfig("overlapping", "cap-micro"))),
    m_enable_debug(false)
{
  const string& debugSwitch = getConfig("debug", "0");
  if (debugSwitch == "1") m_enable_debug = true;

  m_semposMap.clear();
}

SemposScorer::~SemposScorer() {}

void SemposScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  //make sure reference data is clear
  m_ref_sentences.clear();

  //load reference data
  for (size_t rid = 0; rid < referenceFiles.size(); ++rid) {
    ifstream refin(referenceFiles[rid].c_str());
    if (!refin) {
      throw runtime_error("Unable to open: " + referenceFiles[rid]);
    }
    m_ref_sentences.push_back(vector<sentence_t>());
    string line;
    while (getline(refin,line)) {
      line = applyFactors(line);

      str_sentence_t sentence;
      splitSentence(line, sentence);

      sentence_t encodedSentence;
      encodeSentence(sentence, encodedSentence);

      m_ref_sentences[rid].push_back(encodedSentence);
    }
  }
}

void SemposScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  vector<ScoreStatsType> stats;

  const string& sentence = applyFactors(text);
  str_sentence_t splitCandSentence;
  splitSentence(sentence, splitCandSentence);

  sentence_t encodedCandSentence;
  encodeSentence(splitCandSentence, encodedCandSentence);

  if (m_ref_sentences.size() == 1) {
    stats = m_ovr->prepareStats(encodedCandSentence, m_ref_sentences[0][sid]);
  } else {
    float max = -1.0f;
    for (size_t rid = 0; rid < m_ref_sentences.size(); ++rid) {
      const vector<ScoreStatsType>& tmp = m_ovr->prepareStats(encodedCandSentence, m_ref_sentences[rid][sid]);
      if (m_ovr->calculateScore(tmp) > max) {
        stats = tmp;
      }
    }
  }
  entry.set(stats);
}

void SemposScorer::splitSentence(const string& sentence, str_sentence_t& splitSentence)
{
    splitSentence.clear();

    vector<string> tokens;
    split(sentence, ' ', tokens);
    for (vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it)
    {
        vector<string> factors;
        split(*it, '|', factors);
        if (factors.size() != 2) throw runtime_error("Sempos scorer accepts two factors (item|class)");
        const string& item = factors[0];
        const string& klass = factors[1];
        splitSentence.push_back(make_pair(item, klass));
    }
}

void SemposScorer::encodeSentence(const str_sentence_t& sentence, sentence_t& encodedSentence)
{
  for (str_sentence_it it = sentence.begin(); it != sentence.end(); ++it) {
    const int tlemma = encodeString(it->first);
    const int sempos = encodeSempos(it->second);
    if (sempos >= 0) {
      encodedSentence.insert(make_pair(tlemma,sempos));
    }
  }
}

int SemposScorer::encodeString(const string& str)
{
  encoding_it encoding = m_stringMap.find(str);
  int encoded_str;
  if (encoding == m_stringMap.end()) {
    encoded_str = static_cast<int>(m_stringMap.size());
    m_stringMap[str] = encoded_str;
  } else {
    encoded_str = encoding->second;
  }
  return encoded_str;
}

int SemposScorer::encodeSempos(const string& sempos)
{
  if (sempos == "-") return -1;
  encoding_it it = m_semposMap.find(sempos);
  if (it == m_semposMap.end())
  {
    if (m_semposMap.size() == kMaxNOC)
    {
      throw std::runtime_error("Number of classes is greater than kMaxNOC");
    }
    const int classNumber = static_cast<int>(m_semposMap.size());
    m_semposMap[sempos] = classNumber;
    return classNumber;
  }
  else
  {
    return it->second;
  }
}
