#include "SemposOverlapping.h"

#include <algorithm>
#include <stdexcept>

using namespace std;

namespace {

SemposOverlapping* g_overlapping = NULL;

} // namespace

SemposOverlapping* SemposOverlappingFactory::GetOverlapping(const string& str) {
  if (str == "cap-micro") {
    return new CapMicroOverlapping;
  } else if (str == "cap-macro") {
    return new CapMacroOverlapping;
  } else {
    throw runtime_error("Unknown overlapping: " + str);
  }
}

void SemposOverlappingFactory::SetOverlapping(SemposOverlapping* ovr) {
  g_overlapping = ovr;
}

vector<int> CapMicroOverlapping::prepareStats(const sentence_t& cand, const sentence_t& ref)
{
  vector<int> stats(2);
  sentence_t intersection;

  set_intersection(cand.begin(), cand.end(), ref.begin(), ref.end(),
                   inserter(intersection, intersection.begin()));

  stats[0] = static_cast<int>(intersection.size());
  stats[1] = static_cast<int>(ref.size());
  return stats;
}

float CapMicroOverlapping::calculateScore(const vector<int>& stats) const
{
  if (stats.size() != 2) {
    throw std::runtime_error("Size of stats vector has to be 2");
  }
  if (stats[1] == 0) return 1.0f;
  return stats[0] / static_cast<float>(stats[1]);
}

vector<int> CapMacroOverlapping::prepareStats(const sentence_t& cand, const sentence_t& ref)
{
  vector<int> stats(2 * kMaxNOC);
  sentence_t intersection;

  set_intersection(cand.begin(), cand.end(), ref.begin(), ref.end(),
                   inserter(intersection, intersection.begin()));

  for (int i = 0; i < 2 * kMaxNOC; ++i) stats[i] = 0;
  for (sentence_t::const_iterator it = intersection.begin(); it != intersection.end(); ++it) {
    const int sempos = it->second;
    ++stats[2 * sempos];
  }
  for (sentence_t::const_iterator it = ref.begin(); it != ref.end(); ++it) {
    const int sempos = it->second;
    ++stats[2 * sempos + 1];
  }

  return stats;
}

float CapMacroOverlapping::calculateScore(const vector<int>& stats) const
{
  if (stats.size() != 2 * kMaxNOC) {
    // TODO: Add some comments. The number "38" looks like a magic number.
    throw std::runtime_error("Size of stats vector has to be 38");
  }

  int n = 0;
  float sum = 0;
  for (int i = 0; i < kMaxNOC; ++i) {
    int clipped = stats[2 * i];
    int refsize = stats[2 * i + 1];
    if (refsize > 0) {
      sum += clipped / static_cast<float>(refsize);
      ++n;
    }
  }
  if (n == 0) return 1;
  return sum / n;
}
