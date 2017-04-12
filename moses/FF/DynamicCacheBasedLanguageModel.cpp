#include <utility>
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "DynamicCacheBasedLanguageModel.h"

namespace Moses
{

std::map< const std::string, DynamicCacheBasedLanguageModel * > DynamicCacheBasedLanguageModel::s_instance_map;
DynamicCacheBasedLanguageModel *DynamicCacheBasedLanguageModel::s_instance = NULL;

DynamicCacheBasedLanguageModel::DynamicCacheBasedLanguageModel(const std::string &line)
  : StatelessFeatureFunction(1, line)
{
  VERBOSE(2,"Initializing DynamicCacheBasedLanguageModel feature..." << std::endl);

  m_query_type = CBLM_QUERY_TYPE_ALLSUBSTRINGS;
  m_score_type = CBLM_SCORE_TYPE_HYPERBOLA;
  m_maxAge = 1000;
  m_name = "default";
  m_constant = false;

  ReadParameters();
  UTIL_THROW_IF2(s_instance_map.find(m_name) != s_instance_map.end(), "Only 1 DynamicCacheBasedLanguageModel feature named " + m_name + " is allowed");
  s_instance_map[m_name] = this;
  s_instance = this; //for back compatibility

  SetPreComputedScores();
}

DynamicCacheBasedLanguageModel::~DynamicCacheBasedLanguageModel() {};

void DynamicCacheBasedLanguageModel::SetPreComputedScores()
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  precomputedScores.clear();
  for (unsigned int i=0; i<m_maxAge; i++) {
    precomputedScores.push_back(decaying_score(i));
  }

  if ( m_score_type == CBLM_SCORE_TYPE_HYPERBOLA
       || m_score_type == CBLM_SCORE_TYPE_POWER
       || m_score_type == CBLM_SCORE_TYPE_EXPONENTIAL
       || m_score_type == CBLM_SCORE_TYPE_COSINE ) {
    precomputedScores.push_back(decaying_score(m_maxAge));
  } else { // m_score_type = CBLM_SCORE_TYPE_XXXXXXXXX_REWARD
    precomputedScores.push_back(0.0);
  }
  m_lower_score = precomputedScores[m_maxAge];
  VERBOSE(3, "SetPreComputedScores(): lower_age:|" << m_maxAge << "| lower_score:|" << m_lower_score << "|" << std::endl);
}

float DynamicCacheBasedLanguageModel::GetPreComputedScores(const unsigned int age)
{
  VERBOSE(2, "float DynamicCacheBasedLanguageModel::GetPreComputedScores" << std::endl);
  VERBOSE(2, "age:|"<< age << "|" << std::endl);

  if (age < m_maxAge) {
    return precomputedScores.at(age);
  } else {
    VERBOSE(2, "is to big reduced to m)_maxAge:|"<< m_maxAge << "|" << std::endl);
    return precomputedScores.at(m_maxAge);
  }
}

void DynamicCacheBasedLanguageModel::SetParameter(const std::string& key, const std::string& value)
{
  VERBOSE(2, "DynamicCacheBasedLanguageModel::SetParameter key:|" << key << "| value:|" << value << "|" << std::endl);
  if (key == "cblm-query-type") {
    SetQueryType(Scan<size_t>(value));
  } else if (key == "cblm-score-type") {
    SetScoreType(Scan<size_t>(value));
  } else if (key == "cblm-max-age") {
    SetMaxAge(Scan<unsigned int>(value));
  } else if (key == "cblm-file") {
    m_initfiles = Scan<std::string>(value);
  } else if (key == "cblm-name") {
    m_name = Scan<std::string>(value);
  } else if (key == "cblm-constant") {
    m_constant = Scan<bool>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

void DynamicCacheBasedLanguageModel::EvaluateInIsolation(const Phrase &sp
    , const TargetPhrase &tp
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedScores) const
{
  float score = m_lower_score;
  switch(m_query_type) {
  case CBLM_QUERY_TYPE_WHOLESTRING:
    score = Evaluate_Whole_String(tp);
    break;
  case CBLM_QUERY_TYPE_ALLSUBSTRINGS:
    score = Evaluate_All_Substrings(tp);
    break;
  default:
    UTIL_THROW_IF2(false, "This score type (" << m_query_type << ") is unknown.");
  }

  scoreBreakdown.Assign(this, score);
}

float DynamicCacheBasedLanguageModel::Evaluate_Whole_String(const TargetPhrase& tp) const
{
  //consider all words in the TargetPhrase as one n-gram
  // and compute the decaying_score for the whole n-gram
  // and return this value

  decaying_cache_t::const_iterator it;
  float score = m_lower_score;

  std::string w = "";
  size_t endpos = tp.GetSize();
  for (size_t pos = 0 ; pos < endpos ; ++pos) {
    w += tp.GetWord(pos).GetFactor(0)->GetString().as_string();
    if (pos < endpos - 1) {
      w += " ";
    }
  }
  it = m_cache.find(w);

  VERBOSE(4,"cblm::Evaluate_Whole_String: searching w:|" << w << "|" << std::endl);
  if (it != m_cache.end()) { //found!
    score = ((*it).second).second;
    VERBOSE(4,"cblm::Evaluate_Whole_String: found w:|" << w << "|" << std::endl);
  }

  VERBOSE(4,"cblm::Evaluate_Whole_String: returning score:|" << score << "|" << std::endl);
  return score;
}

float DynamicCacheBasedLanguageModel::Evaluate_All_Substrings(const TargetPhrase& tp) const
{
  //loop over all n-grams in the TargetPhrase (no matter of n)
  //and compute the decaying_score for all words
  //and return their sum

  decaying_cache_t::const_iterator it;
  float score = 0.0;

  for (size_t startpos = 0 ; startpos < tp.GetSize() ; ++startpos) {
    std::string w = "";
    for (size_t endpos = startpos; endpos < tp.GetSize() ; ++endpos) {
      w += tp.GetWord(endpos).GetFactor(0)->GetString().as_string();
      it = m_cache.find(w);

      if (it != m_cache.end()) { //found!
        score += ((*it).second).second;
        VERBOSE(3,"cblm::Evaluate_All_Substrings: found w:|" << w << "| actual score:|" << ((*it).second).second << "| score:|" << score << "|" << std::endl);
      } else {
        score += m_lower_score;
      }

      if (endpos == startpos) {
        w += " ";
      }

    }
  }
  VERBOSE(3,"cblm::Evaluate_All_Substrings: returning score:|" << score << "|" << std::endl);
  return score;
}

void DynamicCacheBasedLanguageModel::Print() const
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  decaying_cache_t::const_iterator it;
  std::cout << "Content of the cache of Cache-Based Language Model" << std::endl;
  std::cout << "Size of the cache of Cache-Based Language Model:|" << m_cache.size() << "|" << std::endl;
  for ( it=m_cache.begin() ; it != m_cache.end(); it++ ) {
    std::cout << "word:|" << (*it).first << "| age:|" << ((*it).second).first << "| score:|" << ((*it).second).second << "|" << std::endl;
  }
}

void DynamicCacheBasedLanguageModel::Decay()
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  decaying_cache_t::iterator it;

  unsigned int age;
  float score;
  for ( it=m_cache.begin() ; it != m_cache.end(); it++ ) {
    age=((*it).second).first + 1;
    if (age > m_maxAge) {
      m_cache.erase(it);
      it--;
    } else {
      score = GetPreComputedScores(age);
//      score = decaying_score(age);
      decaying_cache_value_t p (age, score);
      (*it).second = p;
    }
  }
}

void DynamicCacheBasedLanguageModel::Update(std::vector<std::string> words, int age)
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  VERBOSE(3,"words.size():|" << words.size() << "|" << std::endl);
  for (size_t j=0; j<words.size(); j++) {
    words[j] = Trim(words[j]);
//    VERBOSE(3,"CacheBasedLanguageModel::Update   word[" << j << "]:"<< words[j] << " age:" << age << " decaying_score(age):" << decaying_score(age) << std::endl);
//    decaying_cache_value_t p (age,decaying_score(age));
    VERBOSE(3,"CacheBasedLanguageModel::Update   word[" << j << "]:"<< words[j] << " age:" << age << " GetPreComputedScores(age):" << GetPreComputedScores(age) << std::endl);
    decaying_cache_value_t p (age,GetPreComputedScores(age));
    std::pair<std::string, decaying_cache_value_t> e (words[j],p);
    m_cache.erase(words[j]); //always erase the element (do nothing if the entry does not exist)
    m_cache.insert(e); //insert the entry
  }
}

void DynamicCacheBasedLanguageModel::ClearEntries(std::string &entries)
{
  if (entries != "") {
    VERBOSE(3,"entries:|" << entries << "|" << std::endl);
    std::vector<std::string> elements = TokenizeMultiCharSeparator(entries, "||");
    VERBOSE(3,"elements.size() after:|" << elements.size() << "|" << std::endl);
    ClearEntries(elements);
  }
}

void DynamicCacheBasedLanguageModel::ClearEntries(std::vector<std::string> words)
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  VERBOSE(3,"words.size():|" << words.size() << "|" << std::endl);
  for (size_t j=0; j<words.size(); j++) {
    words[j] = Trim(words[j]);
    VERBOSE(3,"CacheBasedLanguageModel::ClearEntries   word[" << j << "]:"<< words[j] << std::endl);
    m_cache.erase(words[j]); //always erase the element (do nothing if the entry does not exist)
  }
}

void DynamicCacheBasedLanguageModel::Insert(std::string &entries)
{
  if (entries != "") {
    VERBOSE(3,"entries:|" << entries << "|" << std::endl);
    std::vector<std::string> elements = TokenizeMultiCharSeparator(entries, "||");
    VERBOSE(3,"elements.size() after:|" << elements.size() << "|" << std::endl);
    Insert(elements);
  }
}

void DynamicCacheBasedLanguageModel::Insert(std::vector<std::string> ngrams)
{
  VERBOSE(3,"DynamicCacheBasedLanguageModel Insert ngrams.size():|" << ngrams.size() << "|" << std::endl);
  if (m_constant == false) {
    Decay();
  }
  Update(ngrams,1);
  IFVERBOSE(3) Print();
}

void DynamicCacheBasedLanguageModel::ExecuteDlt(std::map<std::string, std::string> dlt_meta)
{
  if (dlt_meta.find("cblm") != dlt_meta.end()) {
    Insert(dlt_meta["cblm"]);
  }
  if (dlt_meta.find("cblm-command") != dlt_meta.end()) {
    Execute(dlt_meta["cblm-command"]);
  }
  if (dlt_meta.find("cblm-file") != dlt_meta.end()) {
    Load(dlt_meta["cblm-file"]);
  }
  if (dlt_meta.find("cblm-clear-entries") != dlt_meta.end()) {
    ClearEntries(dlt_meta["cblm-clear-entries"]);
  }
  if (dlt_meta.find("cblm-clear-all") != dlt_meta.end()) {
    Clear();
  }

}

void DynamicCacheBasedLanguageModel::Execute(std::string command)
{
  VERBOSE(2,"DynamicCacheBasedLanguageModel::Execute(std::string command:|" << command << "|" << std::endl);
  std::vector<std::string> commands = Tokenize(command, "||");
  Execute(commands);
}

void DynamicCacheBasedLanguageModel::Execute(std::vector<std::string> commands)
{
  for (size_t j=0; j<commands.size(); j++) {
    Execute_Single_Command(commands[j]);
  }
  IFVERBOSE(2) Print();
}

void DynamicCacheBasedLanguageModel::Execute_Single_Command(std::string command)
{
  VERBOSE(2,"CacheBasedLanguageModel::Execute_Single_Command(std::string command:|" << command << "|" << std::endl);
  if (command == "clear") {
    VERBOSE(2,"CacheBasedLanguageModel Execute command:|"<< command << "|. Cache cleared." << std::endl);
    Clear();
  } else if (command == "settype_wholestring") {
    VERBOSE(2,"CacheBasedLanguageModel Execute command:|"<< command << "|. Query type set to " << CBLM_QUERY_TYPE_WHOLESTRING << " (CBLM_QUERY_TYPE_WHOLESTRING)." << std::endl);
    SetQueryType(CBLM_QUERY_TYPE_WHOLESTRING);
  } else if (command == "settype_allsubstrings") {
    VERBOSE(2,"CacheBasedLanguageModel Execute command:|"<< command << "|. Query type set to " << CBLM_QUERY_TYPE_ALLSUBSTRINGS << " (CBLM_QUERY_TYPE_ALLSUBSTRINGS)." << std::endl);
    SetQueryType(CBLM_QUERY_TYPE_ALLSUBSTRINGS);
  } else {
    VERBOSE(2,"CacheBasedLanguageModel Execute command:|"<< command << "| is unknown. Skipped." << std::endl);
  }
}

void DynamicCacheBasedLanguageModel::Clear()
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> lock(m_cacheLock);
#endif
  m_cache.clear();
}

void DynamicCacheBasedLanguageModel::Load(AllOptions::ptr const& opts)
{
  m_options = opts;
//  SetPreComputedScores();
  VERBOSE(2,"DynamicCacheBasedLanguageModel::Load()" << std::endl);
  Load(m_initfiles);
}

void DynamicCacheBasedLanguageModel::Load(const std::string filestr)
{
  VERBOSE(2,"DynamicCacheBasedLanguageModel::Load(const std::string filestr)" << std::endl);
//  std::vector<std::string> files = Tokenize(m_initfiles, "||");
  std::vector<std::string> files = Tokenize(filestr, "||");
  Load_Multiple_Files(files);
}


void DynamicCacheBasedLanguageModel::Load_Multiple_Files(std::vector<std::string> files)
{
  VERBOSE(2,"DynamicCacheBasedLanguageModel::Load_Multiple_Files(std::vector<std::string> files)" << std::endl);
  for(size_t j = 0; j < files.size(); ++j) {
    Load_Single_File(files[j]);
  }
}

void DynamicCacheBasedLanguageModel::Load_Single_File(const std::string file)
{
  VERBOSE(2,"DynamicCacheBasedLanguageModel::Load_Single_File(const std::string file)" << std::endl);
  //file format
  //age || n-gram
  //age || n-gram || n-gram || n-gram || ...
  //....
  //each n-gram is a sequence of n words (no matter of n)
  //
  //there is no limit on the size of n
  //
  //entries can be repeated, but the last entry overwrites the previous


  VERBOSE(2,"Loading data from the cache file " << file << std::endl);
  InputFileStream cacheFile(file);

  std::string line;
  int age;
  std::vector<std::string> words;

  while (getline(cacheFile, line)) {
    std::vector<std::string> vecStr = TokenizeMultiCharSeparator( line , "||" );
    if (vecStr.size() >= 2) {
      age = Scan<int>(vecStr[0]);
      vecStr.erase(vecStr.begin());
      Update(vecStr,age);
    } else {
      UTIL_THROW_IF2(false, "The format of the loaded file is wrong: " << line);
    }
  }
  IFVERBOSE(2) Print();
}

void DynamicCacheBasedLanguageModel::SetQueryType(size_t type)
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif

  m_query_type = type;
  if ( m_query_type != CBLM_QUERY_TYPE_WHOLESTRING
       && m_query_type != CBLM_QUERY_TYPE_ALLSUBSTRINGS ) {
    VERBOSE(2, "This query type " << m_query_type << " is unknown. Instead used " << CBLM_QUERY_TYPE_ALLSUBSTRINGS << "." << std::endl);
    m_query_type = CBLM_QUERY_TYPE_ALLSUBSTRINGS;
  }
  VERBOSE(2, "CacheBasedLanguageModel QueryType:  " << m_query_type << std::endl);

};

void DynamicCacheBasedLanguageModel::SetScoreType(size_t type)
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  m_score_type = type;
  if ( m_score_type != CBLM_SCORE_TYPE_HYPERBOLA
       && m_score_type != CBLM_SCORE_TYPE_POWER
       && m_score_type != CBLM_SCORE_TYPE_EXPONENTIAL
       && m_score_type != CBLM_SCORE_TYPE_COSINE
       && m_score_type != CBLM_SCORE_TYPE_HYPERBOLA_REWARD
       && m_score_type != CBLM_SCORE_TYPE_POWER_REWARD
       && m_score_type != CBLM_SCORE_TYPE_EXPONENTIAL_REWARD ) {
    VERBOSE(2, "This score type " << m_score_type << " is unknown. Instead used " << CBLM_SCORE_TYPE_HYPERBOLA << "." << std::endl);
    m_score_type = CBLM_SCORE_TYPE_HYPERBOLA;
  }
  VERBOSE(2, "CacheBasedLanguageModel ScoreType:  " << m_score_type << std::endl);
};

void DynamicCacheBasedLanguageModel::SetMaxAge(unsigned int age)
{
#ifdef WITH_THREADS
  boost::shared_lock<boost::shared_mutex> read_lock(m_cacheLock);
#endif
  m_maxAge = age;
  VERBOSE(2, "CacheBasedLanguageModel MaxAge:  " << m_maxAge << std::endl);
};

float DynamicCacheBasedLanguageModel::decaying_score(const unsigned int age)
{
  float sc;
  switch(m_score_type) {
  case CBLM_SCORE_TYPE_HYPERBOLA:
    sc = (float) 1.0/age - 1.0;
    break;
  case CBLM_SCORE_TYPE_POWER:
    sc = (float) pow(age, -0.25) - 1.0;
    break;
  case CBLM_SCORE_TYPE_EXPONENTIAL:
    sc = (age == 1) ? 0.0 : (float) exp( 1.0/age ) / exp(1.0) - 1.0;
    break;
  case CBLM_SCORE_TYPE_COSINE:
    sc = (float) cos( (age-1) * (PI/2) / m_maxAge ) - 1.0;
    break;
  case CBLM_SCORE_TYPE_HYPERBOLA_REWARD:
    sc = (float) 1.0/age;
    break;
  case CBLM_SCORE_TYPE_POWER_REWARD:
    sc = (float) pow(age, -0.25);
    break;
  case CBLM_SCORE_TYPE_EXPONENTIAL_REWARD:
    sc = (age == 1) ? 1.0 : (float) exp( 1.0/age ) / exp(1.0);
    break;
  default:
    sc = -1.0;
  }
  return sc;
}
}
