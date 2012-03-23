#include <sys/types.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <map>
#include <iterator>

#include "SemposScorer.h"
#include "Util.h"

SemposScorer::SemposScorer(const string& config)
  : StatisticsBasedScorer("SEMPOS",config),
    debug(false)
{
  string debugSwitch = getConfig("debug", "0");
  if (debugSwitch == "1") debug = true;
  
  string overlapping = getConfig("overlapping", "cap-micro");
  if (overlapping == "cap-micro") {
    ovr = new CapMicroOverlapping();
  } else if (overlapping == "cap-macro") {
    ovr = new CapMacroOverlapping();
  } else {
    throw runtime_error("Unknown overlapping: " + overlapping);
  }

  semposMap.clear();
}

void SemposScorer::setReferenceFiles(const vector<string>& referenceFiles)
{
  //make sure reference data is clear
  ref_sentences.clear();

  //load reference data
  for (size_t rid = 0; rid < referenceFiles.size(); ++rid) {
    ifstream refin(referenceFiles[rid].c_str());
    if (!refin) {
      throw runtime_error("Unable to open: " + referenceFiles[rid]);
    }
    ref_sentences.push_back(vector<sentence_t>());
    string line;
    while (getline(refin,line)) {
      line = applyFactors(line); 

      str_sentence_t sentence;
      splitSentence(line, sentence);

      sentence_t encodedSentence;
      encodeSentence(sentence, encodedSentence);

      ref_sentences[rid].push_back(encodedSentence);
    }
  }
}

void SemposScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry)
{
  vector<int> stats;  

  string sentence = applyFactors(text);

  str_sentence_t splitCandSentence;
  splitSentence(sentence, splitCandSentence);

  sentence_t encodedCandSentence;
  encodeSentence(splitCandSentence, encodedCandSentence);

  if (ref_sentences.size() == 1) {
    stats = ovr->prepareStats(encodedCandSentence, ref_sentences[0][sid]);
  } else {
    float max = -1;
    for (size_t rid = 0; rid < ref_sentences.size(); ++rid) {
      vector<int> tmp = ovr->prepareStats(encodedCandSentence, ref_sentences[rid][sid]);
      if (ovr->calculateScore(tmp) > max) {
        stats = tmp;
      }
    }
  }

  stringstream sout;
  copy(stats.begin(),stats.end(),ostream_iterator<int>(sout," "));
  string stats_str = sout.str();
  entry.set(stats_str);
}

float SemposScorer::calculateScore(const vector<int>& comps) const
{
  return ovr->calculateScore(comps);
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
        string Item = factors[0];
        string Class = factors[1];
        splitSentence.push_back(make_pair(Item, Class));
    }
}

void SemposScorer::encodeSentence(const str_sentence_t& sentence, sentence_t& encodedSentence)
{
  for (str_sentence_it it = sentence.begin(); it != sentence.end(); ++it) {
    int tlemma = encodeString(it->first);
    int sempos = encodeSempos(it->second);
    if (sempos >= 0) {
      encodedSentence.insert(make_pair(tlemma,sempos));
    }
  }
}

int SemposScorer::encodeString(const string& str)
{
  encoding_it encoding = stringMap.find(str);
  int encoded_str;
  if (encoding == stringMap.end()) {
    encoded_str = (int)stringMap.size();
    stringMap[str] = encoded_str;
  } else {
    encoded_str = encoding->second;
  }
  return encoded_str;
}

int SemposScorer::encodeSempos(const string& sempos)
{
  if (sempos == "-") return -1;
  encoding_it it = semposMap.find(sempos);
  if (it == semposMap.end())
  {
    if (semposMap.size() == maxNOC)
    {
      throw std::runtime_error("Number of classes is greater than maxNOC");
    }
    int classNumber = semposMap.size();
    semposMap[sempos] = classNumber;
    return classNumber;
  }
  else
  {
    return it->second;
  }
}

SemposScorer::~SemposScorer()
{
  delete ovr;
}

vector<int> CapMicroOverlapping::prepareStats(const sentence_t& cand, const sentence_t& ref)
{
  vector<int> stats(2);
  sentence_t intersection;

  set_intersection(cand.begin(),cand.end(),ref.begin(),ref.end(), inserter(intersection, intersection.begin()));

  stats[0] = intersection.size();
  stats[1] = ref.size();
  return stats;
}

float CapMicroOverlapping::calculateScore(const vector<int>& stats)
{
  if (stats.size() != 2)
  {
    throw std::runtime_error("Size of stats vector has to be 2");
  }
  if (stats[1] == 0) return (float) 1;
  return stats[0]/(float)stats[1];
}


vector<int> CapMacroOverlapping::prepareStats(const sentence_t& cand, const sentence_t& ref)
{
  vector<int> stats(2*maxNOC);
  sentence_t intersection;

  set_intersection(cand.begin(),cand.end(),ref.begin(),ref.end(), inserter(intersection, intersection.begin()));

  for (int i = 0; i < 2*maxNOC; ++i) stats[i]=0;
  for (sentence_t::const_iterator it = intersection.begin(); it != intersection.end(); ++it) {
    int sempos = it->second;
    ++stats[2*sempos];
  }
  for (sentence_t::const_iterator it = ref.begin(); it != ref.end(); ++it) {
    int sempos = it->second;
    ++stats[2*sempos+1];
  }

  return stats;
}

float CapMacroOverlapping::calculateScore(const vector<int>& stats)
{
  if (stats.size() != 2*maxNOC) throw std::runtime_error("Size of stats vector has to be 38");

  int n = 0;
  float sum = 0;
  for (int i = 0; i < maxNOC; ++i) {
    int clipped = stats[2*i];
    int refsize = stats[2*i+1];
    if (refsize > 0) {
      sum += clipped / (float) refsize;
      ++n;
    }
  }
  if (n == 0) return 1;
  return sum / n;
}

