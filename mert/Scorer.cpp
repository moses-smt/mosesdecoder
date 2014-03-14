#include "Scorer.h"

#include <limits>
#include "Vocabulary.h"
#include "Util.h"
#include "Singleton.h"
#include "util/tokenize_piece.hh"

#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
#include "PreProcessFilter.h"
#endif

using namespace std;

namespace MosesTuning
{

namespace
{
// For tokenizing a hypothesis translation, we may encounter unknown tokens which
// do not exist in the corresponding reference translations.
const int kUnknownToken = -1;
} // namespace

Scorer::Scorer(const string& name, const string& config)
  : m_name(name),
    m_vocab(mert::VocabularyFactory::GetVocabulary()),
    #if defined(__GLIBCXX__) || defined(__GLIBCPP__)
    m_filter(NULL),
    #endif
    m_score_data(NULL),
    m_enable_preserve_case(true)
{
  InitConfig(config);
}

Scorer::~Scorer()
{
  Singleton<mert::Vocabulary>::Delete();
#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
  delete m_filter;
#endif
}

void Scorer::InitConfig(const string& config)
{
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

void Scorer::TokenizeAndEncode(const string& line, vector<int>& encoded)
{
  for (util::TokenIter<util::AnyCharacter, true> it(line, util::AnyCharacter(" "));
       it; ++it) {
    if (!m_enable_preserve_case) {
      string token = it->as_string();
      for (std::string::iterator sit = token.begin();
           sit != token.end(); ++sit) {
        *sit = tolower(*sit);
      }
      encoded.push_back(m_vocab->Encode(token));
    } else {
      encoded.push_back(m_vocab->Encode(it->as_string()));
    }
  }
}

void Scorer::TokenizeAndEncodeTesting(const string& line, vector<int>& encoded)
{
  for (util::TokenIter<util::AnyCharacter, true> it(line, util::AnyCharacter(" "));
       it; ++it) {
    if (!m_enable_preserve_case) {
      string token = it->as_string();
      for (std::string::iterator sit = token.begin();
           sit != token.end(); ++sit) {
        *sit = tolower(*sit);
      }
      mert::Vocabulary::const_iterator cit = m_vocab->find(token);
      if (cit == m_vocab->end()) {
        encoded.push_back(kUnknownToken);
      } else {
        encoded.push_back(cit->second);
      }
    } else {
      mert::Vocabulary::const_iterator cit = m_vocab->find(it->as_string());
      if (cit == m_vocab->end()) {
        encoded.push_back(kUnknownToken);
      } else {
        encoded.push_back(cit->second);
      }
    }
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
  for(vector<string>::iterator it = factors_vec.begin(); it != factors_vec.end(); ++it) {
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
#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
  m_filter = new PreProcessFilter(filterCommand);
#else
  throw runtime_error("Cannot use filter command as mert was compiled with non-gcc compiler");
#endif
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
  for (size_t i = 0; i < tokens.size(); ++i) {
    if (tokens[i] == "") continue;

    vector<string> factors;
    split(tokens[i], '|', factors);

    int fsize = factors.size();

    if (i > 0) sstream << " ";

    for (size_t j = 0; j < m_factors.size(); ++j) {
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
#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
  if (m_filter) {
    return m_filter->ProcessSentence(sentence);
  } else {
    return sentence;
  }
#endif
  return sentence;
}

float Scorer::score(const candidates_t& candidates) const
{
  diffs_t diffs;
  statscores_t scores;
  score(candidates, diffs, scores);
  return scores[0];
}

}
