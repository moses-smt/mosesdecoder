#include "CderScorer.h"
#include <fstream>
#include <stdexcept>

CderScorer::CderScorer(const string& config)
    : StatisticsBasedScorer("CDER",config) {}

CderScorer::~CderScorer() {}

void CderScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  //make sure reference data is clear
  ref_sentences.clear();

  //load reference data
  for (size_t rid = 0; rid < referenceFiles.size(); ++rid) {
    ifstream refin(referenceFiles[rid].c_str());
    if (!refin) {
      throw runtime_error("Unable to open: " + referenceFiles[rid]);
    }
    ref_sentences.push_back(vector<sent_t>());
    string line;
    while (getline(refin,line)) {
      sent_t encoded;
      encode(line, encoded);
      ref_sentences[rid].push_back(encoded);
    }
  }
}

void CderScorer::prepareStatsVector(size_t sid, const string& text, vector<int>& stats)
{
  sent_t cand;
  encode(text, cand);

  float max = -2;
  for (size_t rid = 0; rid < ref_sentences.size(); ++rid) {
    sent_t& ref = ref_sentences[rid][sid];
    vector<int> tmp = computeCD(cand, ref);
    if (calculateScore(tmp) > max) {
      stats = tmp;
    }
  }
}

float CderScorer::calculateScore(const vector<int>& comps) const
{
  if (comps.size() != 2)
  {
    throw runtime_error("Size of stat vector for CDER is not 2");
  }

  return 1 - (comps[0] / (float) comps[1]);
}

vector<int> CderScorer::computeCD(const sent_t& cand, const sent_t& ref) const
{
  int I = cand.size() + 1; // Number of inter-words positions in candidate sentence
  int L = ref.size() + 1; // Number of inter-words positions in reference sentence

  int l = 0;
  // row[i] stores cost of cheapest path from (0,0) to (i,l) in CDER aligment grid.
  vector<int>* row = new vector<int>(I);

  // Initialization of first row
  (*row)[0] = 0;
  for (int i = 1; i < I; ++i) (*row)[i] = 1;

  // Calculating costs for next row using costs from the previous row.
  while (++l < L)
  {
    vector<int>* nextRow = new vector<int>(I);
    for (int i = 0; i < I; ++i)
    {
      vector<int> possibleCosts;
      if (i > 0) {
        possibleCosts.push_back((*nextRow)[i-1] + 1); // Deletion
        possibleCosts.push_back((*row)[i-1] + distance(ref[l-1], cand[i-1])); // Substitution/Identity
      }
      possibleCosts.push_back((*row)[i] + 1); // Insertion
      (*nextRow)[i] = *min_element(possibleCosts.begin(), possibleCosts.end());
    }

    // Cost of LongJumps is the same for all in the row
    int LJ = 1 + *min_element(nextRow->begin(), nextRow->end());

    for (int i = 0; i < I; ++i) {
      (*nextRow)[i] = min((*nextRow)[i], LJ); // LongJumps
    }

    delete row;
    row = nextRow;
  }

  vector<int> stats(2);
  stats[0] = *(row->rbegin());  // CD distance is the cost of path from (0,0) to (I,L)
  stats[1] = ref.size();

  delete row;
  return stats;
}
