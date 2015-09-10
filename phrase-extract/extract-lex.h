#pragma once

#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <iostream>

namespace MosesTraining
{

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
