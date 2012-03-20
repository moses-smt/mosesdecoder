#include "Scorer.h"

#include <limits>
#include "Vocabulary.h"
#include "Util.h"
#include "Singleton.h"

namespace {

//regularisation strategies
inline float score_min(const statscores_t& scores, size_t start, size_t end)
{
  float min = numeric_limits<float>::max();
  for (size_t i = start; i < end; ++i) {
    if (scores[i] < min) {
      min = scores[i];
    }
  }
  return min;
}

inline float score_average(const statscores_t& scores, size_t start, size_t end)
{
  if ((end - start) < 1) {
    // this shouldn't happen
    return 0;
  }
  float total = 0;
  for (size_t j = start; j < end; ++j) {
    total += scores[j];
  }

  return total / (end - start);
}

} // namespace

Scorer::Scorer(const string& name, const string& config)
    : m_name(name),
      m_vocab(mert::VocabularyFactory::GetVocabulary()),
      m_score_data(0),
      m_enable_preserve_case(true) {
  InitConfig(config);
}

Scorer::~Scorer() {
  Singleton<mert::Vocabulary>::Delete();
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

StatisticsBasedScorer::StatisticsBasedScorer(const string& name, const string& config)
    : Scorer(name,config) {
  //configure regularisation
  static string KEY_TYPE = "regtype";
  static string KEY_WINDOW = "regwin";
  static string KEY_CASE = "case";
  static string TYPE_NONE = "none";
  static string TYPE_AVERAGE = "average";
  static string TYPE_MINIMUM = "min";
  static string TRUE = "true";
  static string FALSE = "false";

  string type = getConfig(KEY_TYPE,TYPE_NONE);
  if (type == TYPE_NONE) {
    m_regularization_type = NONE;
  } else if (type == TYPE_AVERAGE) {
    m_regularization_type = AVERAGE;
  } else if (type == TYPE_MINIMUM) {
    m_regularization_type = MINIMUM;
  } else {
    throw runtime_error("Unknown scorer regularisation strategy: " + type);
  }
  //    cerr << "Using scorer regularisation strategy: " << type << endl;

  const string& window = getConfig(KEY_WINDOW, "0");
  m_regularization_window = atoi(window.c_str());
  //    cerr << "Using scorer regularisation window: " << m_regularization_window << endl;

  const string& preserve_case = getConfig(KEY_CASE,TRUE);
  if (preserve_case == TRUE) {
    m_enable_preserve_case = true;
  } else if (preserve_case == FALSE) {
    m_enable_preserve_case = false;
  }
  //    cerr << "Using case preservation: " << m_enable_preserve_case << endl;
}

void  StatisticsBasedScorer::score(const candidates_t& candidates, const diffs_t& diffs,
                                   statscores_t& scores) const
{
  if (!m_score_data) {
    throw runtime_error("Score data not loaded");
  }
  // calculate the score for the candidates
  if (m_score_data->size() == 0) {
    throw runtime_error("Score data is empty");
  }
  if (candidates.size() == 0) {
    throw runtime_error("No candidates supplied");
  }
  int numCounts = m_score_data->get(0,candidates[0]).size();
  vector<int> totals(numCounts);
  for (size_t i = 0; i < candidates.size(); ++i) {
    ScoreStats stats = m_score_data->get(i,candidates[i]);
    if (stats.size() != totals.size()) {
      stringstream msg;
      msg << "Statistics for (" << "," << candidates[i] << ") have incorrect "
          << "number of fields. Found: " << stats.size() << " Expected: "
          << totals.size();
      throw runtime_error(msg.str());
    }
    for (size_t k = 0; k < totals.size(); ++k) {
      totals[k] += stats.get(k);
    }
  }
  scores.push_back(calculateScore(totals));

  candidates_t last_candidates(candidates);
  // apply each of the diffs, and get new scores
  for (size_t i = 0; i < diffs.size(); ++i) {
    for (size_t j = 0; j < diffs[i].size(); ++j) {
      size_t sid = diffs[i][j].first;
      size_t nid = diffs[i][j].second;
      size_t last_nid = last_candidates[sid];
      for (size_t k  = 0; k < totals.size(); ++k) {
        int diff = m_score_data->get(sid,nid).get(k)
                   - m_score_data->get(sid,last_nid).get(k);
        totals[k] += diff;
      }
      last_candidates[sid] = nid;
    }
    scores.push_back(calculateScore(totals));
  }

  // Regularisation. This can either be none, or the min or average as described in
  // Cer, Jurafsky and Manning at WMT08.
  if (m_regularization_type == NONE || m_regularization_window <= 0) {
    // no regularisation
    return;
  }

  // window size specifies the +/- in each direction
  statscores_t raw_scores(scores);      // copy scores
  for (size_t i = 0; i < scores.size(); ++i) {
    size_t start = 0;
    if (i >= m_regularization_window) {
      start = i - m_regularization_window;
    }
    const size_t end = min(scores.size(), i + m_regularization_window + 1);
    if (m_regularization_type == AVERAGE) {
      scores[i] = score_average(raw_scores,start,end);
    } else {
      scores[i] = score_min(raw_scores,start,end);
    }
  }
}
