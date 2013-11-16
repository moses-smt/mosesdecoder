#pragma once

#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <iostream>

namespace MosesTraining
{


//! convert string to variable of type T. Used to reading floats, int etc from files
template<typename T>
inline T Scan(const std::string &input)
{
  std::stringstream stream(input);
  T ret;
  stream >> ret;
  return ret;
}


//! speeded up version of above
template<typename T>
inline void Scan(std::vector<T> &output, const std::vector< std::string > &input)
{
  output.resize(input.size());
  for (size_t i = 0 ; i < input.size() ; i++) {
    output[i] = Scan<T>( input[i] );
  }
}


inline void Tokenize(std::vector<std::string> &output
                     , const std::string& str
                     , const std::string& delimiters = " \t")
{
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    output.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
}

// speeded up version of above
template<typename T>
inline void Tokenize( std::vector<T> &output
                      , const std::string &input
                      , const std::string& delimiters = " \t")
{
  std::vector<std::string> stringVector;
  Tokenize(stringVector, input, delimiters);
  return Scan<T>(output, stringVector );
}

class WordCount
{
  friend std::ostream& operator<<(std::ostream&, const WordCount&);
public:
  float m_count;

  std::map<const std::string*, WordCount> m_coll;

  WordCount()
    :m_count(0) {
  }

  //WordCount(const WordCount &copy);

  WordCount(float count)
    :m_count(count) {
  }

  void AddCount(float incr);

  std::map<const std::string*, WordCount> &GetColl() {
    return m_coll;
  }
  const std::map<const std::string*, WordCount> &GetColl() const {
    return m_coll;
  }

  const float GetCount() const {
    return m_count;
  }

};

class Vocab
{
  std::set<std::string> m_coll;
public:
  const std::string *GetOrAdd(const std::string &word);
};

class ExtractLex
{
  Vocab m_vocab;
  std::map<const std::string*, WordCount> m_collS2T, m_collT2S;

  void Process(const std::string *target, const std::string *source);
  void Process(WordCount &wcIn, const std::string *out);
  void ProcessUnaligned(std::vector<std::string> &toksTarget, std::vector<std::string> &toksSource
                        , const std::vector<bool> &m_sourceAligned, const std::vector<bool> &m_targetAligned);

  void Output(const std::map<const std::string*, WordCount> &coll, std::ofstream &outStream);

public:
  void Process(std::vector<std::string> &toksTarget, std::vector<std::string> &toksSource, std::vector<std::string> &toksAlign, size_t lineCount);
  void Output(std::ofstream &streamLexS2T, std::ofstream &streamLexT2S);

};

} // namespace
