#include "CderScorer.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

using namespace std;

namespace
{

inline int CalcDistance(int word1, int word2)
{
  return word1 == word2 ? 0 : 1;
}

} // namespace

namespace MosesTuning
{


CderScorer::CderScorer(const string& config, bool allowed_long_jumps)
  : StatisticsBasedScorer(allowed_long_jumps ? "CDER" : "WER", config),
    m_allowed_long_jumps(allowed_long_jumps) {}

CderScorer::~CderScorer() {}

void CderScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  //make sure reference data is clear
  m_ref_sentences.clear();

  //load reference data
  for (size_t rid = 0; rid < referenceFiles.size(); ++rid) {
    ifstream refin(referenceFiles[rid].c_str());
    if (!refin) {
      throw runtime_error("Unable to open: " + referenceFiles[rid]);
    }
    m_ref_sentences.push_back(vector<sent_t>());
    string line;
    while (getline(refin,line)) {
      line = this->preprocessSentence(line);
      sent_t encoded;
      TokenizeAndEncode(line, encoded);
      m_ref_sentences[rid].push_back(encoded);
    }
  }
}

void CderScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  string sentence = this->preprocessSentence(text);

  vector<int> stats;
  prepareStatsVector(sid, sentence, stats);
  entry.set(stats);
}

void CderScorer::prepareStatsVector(size_t sid, const string& text, vector<int>& stats)
{
  sent_t cand;
  TokenizeAndEncode(text, cand);

  float max = -2;
  vector<int> tmp;
  for (size_t rid = 0; rid < m_ref_sentences.size(); ++rid) {
    const sent_t& ref = m_ref_sentences[rid][sid];
    tmp.clear();
    computeCD(cand, ref, tmp);
    int score = calculateScore(tmp);
    if (rid == 0) {
      stats = tmp;
      max = score;
    } else if (score > max) {
      stats = tmp;
      max = score;
    }
  }
}

float CderScorer::calculateScore(const vector<int>& comps) const
{
  if (comps.size() != 2) {
    throw runtime_error("Size of stat vector for CDER is not 2");
  }
  if (comps[1] == 0) return 1.0f;
  return 1.0f - (comps[0] / static_cast<float>(comps[1]));
}

void CderScorer::computeCD(const sent_t& cand, const sent_t& ref,
                           vector<int>& stats) const
{
  int I = cand.size() + 1; // Number of inter-words positions in candidate sentence
  int L = ref.size() + 1; // Number of inter-words positions in reference sentence

  int l = 0;
  // row[i] stores cost of cheapest path from (0,0) to (i,l) in CDER aligment grid.
  vector<int>* row = new vector<int>(I);

  // Initialization of first row
  for (int i = 0; i < I; ++i) (*row)[i] = i;

  // For CDER metric, the initialization is different
  if (m_allowed_long_jumps) {
    for (int i = 1; i < I; ++i) (*row)[i] = 1;
  }

  // Calculating costs for next row using costs from the previous row.
  while (++l < L) {
    vector<int>* nextRow = new vector<int>(I);
    for (int i = 0; i < I; ++i) {
      vector<int> possibleCosts;
      if (i > 0) {
        possibleCosts.push_back((*nextRow)[i-1] + 1); // Deletion
        possibleCosts.push_back((*row)[i-1] + CalcDistance(ref[l-1], cand[i-1])); // Substitution/Identity
      }
      possibleCosts.push_back((*row)[i] + 1); // Insertion
      (*nextRow)[i] = *min_element(possibleCosts.begin(), possibleCosts.end());
    }

    if (m_allowed_long_jumps) {
      // Cost of LongJumps is the same for all in the row
      int LJ = 1 + *min_element(nextRow->begin(), nextRow->end());

      for (int i = 0; i < I; ++i) {
        (*nextRow)[i] = min((*nextRow)[i], LJ); // LongJumps
      }
    }

    delete row;
    row = nextRow;
  }

  stats.resize(2);
  stats[0] = *(row->rbegin());  // CD distance is the cost of path from (0,0) to (I,L)
  stats[1] = ref.size();

  delete row;
}

}
