#ifndef MERT_VOCABULARY_H_
#define MERT_VOCABULARY_H_

#include <boost/unordered_map.hpp>
#include <string>

namespace mert
{

/**
 * A map to handle vocabularies to calculate
 * various scores such as BLEU.
 *
 * TODO: replace this with more efficient data structure.
 */
class Vocabulary
{
public:
  typedef boost::unordered_map<std::string, int>::iterator iterator;
  typedef boost::unordered_map<std::string, int>::const_iterator const_iterator;

  Vocabulary() {}
  virtual ~Vocabulary() {}

  /** Returns the assiged id for given "token". */
  int Encode(const std::string& token);

  /**
   * Return true iff the specified "str" is found in the container.
   */
  bool Lookup(const std::string&str , int* v) const;

  void clear() {
    m_vocab.clear();
  }

  bool empty() const {
    return m_vocab.empty();
  }

  std::size_t size() const {
    return m_vocab.size();
  }

  iterator find(const std::string& str) {
    return m_vocab.find(str);
  }
  const_iterator find(const std::string& str) const {
    return m_vocab.find(str);
  }

  int& operator[](const std::string& str) {
    return m_vocab[str];
  }

  iterator begin() {
    return m_vocab.begin();
  }
  const_iterator begin() const {
    return m_vocab.begin();
  }
  iterator end() {
    return m_vocab.end();
  }
  const_iterator end() const {
    return m_vocab.end();
  }

private:
  boost::unordered_map<std::string, int> m_vocab;
};

class VocabularyFactory
{
public:
  static Vocabulary* GetVocabulary();
  static void SetVocabulary(Vocabulary* vocab);

private:
  VocabularyFactory() {}
  virtual ~VocabularyFactory() {}
};

} // namespace mert

#endif  // MERT_VOCABULARY_H_
