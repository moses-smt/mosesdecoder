#include "Scorer.h"

#include <limits>
#include "Vocabulary.h"
#include "Util.h"
#include "Singleton.h"
#include "PreProcessFilter.h"

using namespace std;

namespace MosesTuning
{
  

Scorer::Scorer(const string& name, const string& config)
    : m_name(name),
      m_vocab(mert::VocabularyFactory::GetVocabulary()),
      m_filter(NULL),
      m_score_data(NULL),
      m_enable_preserve_case(true) {
  InitConfig(config);
}

Scorer::~Scorer() {
  Singleton<mert::Vocabulary>::Delete();
  delete m_filter;
}

void Scorer::InitConfig(const string& config) {
//    cerr << "Scorer config string: " << config << endl;
  size_t start = 0;
  while (start < config.size()) {
    size_t end = config.find(",", start);
    if (end == string::npos) {
      end = config.size();
    }
    string nv = config.substr(start, end - start);
    size_t split = nv.find(":");
    if (split == string::npos) {
      throw runtime_error("Missing colon when processing scorer config: " + config);
    }
    const string name = nv.substr(0, split);
    const string value = nv.substr(split + 1, nv.size() - split - 1);
    cerr << "name: " << name << " value: " << value << endl;
    m_config[name] = value;
    start = end + 1;
  }
}

void Scorer::TokenizeAndEncode(const string& line, vector<int>& encoded) {
  std::istringstream in(line);
  std::string token;
  while (in >> token) {
    if (!m_enable_preserve_case) {
      for (std::string::iterator it = token.begin();
           it != token.end(); ++it) {
        *it = tolower(*it);
      }
    }
    encoded.push_back(m_vocab->Encode(token));
  }
}

/**
 * Set the factors, which should be used for this metric
 */
void Scorer::setFactors(const string& factors)
{
  if (factors.empty()) return;
  vector<string> factors_vec;
  split(factors, '|', factors_vec);
  for(vector<string>::iterator it = factors_vec.begin(); it != factors_vec.end(); ++it)
  {
    int factor = atoi(it->c_str());
    m_factors.push_back(factor);
  }
}

/**
 * Set unix filter, which will be used to preprocess the sentences
 */
void Scorer::setFilter(const string& filterCommand)
{
    if (filterCommand.empty()) return;
    m_filter = new PreProcessFilter(filterCommand);
}

/**
 * Take the factored sentence and return the desired factors
 */
string Scorer::applyFactors(const string& sentence) const
{
  if (m_factors.size() == 0) return sentence;

  vector<string> tokens;
  split(sentence, ' ', tokens);

  stringstream sstream;
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    if (tokens[i] == "") continue;

    vector<string> factors;
    split(tokens[i], '|', factors);

    int fsize = factors.size();

    if (i > 0) sstream << " ";

    for (size_t j = 0; j < m_factors.size(); ++j)
    {
      int findex = m_factors[j];
      if (findex < 0 || findex >= fsize) throw runtime_error("Factor index is out of range.");

      if (j > 0) sstream << "|";
      sstream << factors[findex];
    }
  }
  return sstream.str();
}

/**
 * Preprocess the sentence with the filter (if given)
 */
string Scorer::applyFilter(const string& sentence) const
{
  if (m_filter)
  {
    return m_filter->ProcessSentence(sentence);
  }
  else
  {
    return sentence;
  }
}

float Scorer::score(const candidates_t& candidates) const {
  diffs_t diffs;
  statscores_t scores;
  score(candidates, diffs, scores);
  return scores[0];
}

}
