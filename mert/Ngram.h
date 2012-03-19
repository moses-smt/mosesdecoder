#ifndef MERT_NGRAM_H_
#define MERT_NGRAM_H_

#include <vector>
#include <map>
#include <string>

/**
 * A map to manage vocaburaries.
 */
class Encoder {
 public:
  typedef std::map<std::string, int>::iterator iterator;
  typedef std::map<std::string, int>::const_iterator const_iterator;

  Encoder() {}
  virtual ~Encoder() {}

  /** Returns the assiged id for given "token". */
  int Encode(const std::string& token) {
    iterator it = m_vocab.find(token);
    int encoded_token;
    if (it == m_vocab.end()) {
      // Add an new entry to the vocaburary.
      encoded_token = static_cast<int>(m_vocab.size());
      m_vocab[token] = encoded_token;
    } else {
      encoded_token = it->second;
    }
    return encoded_token;
  }

  /**
   * Return true iff the specified "str" is found in the container.
   */
  bool Lookup(const std::string&str , int* v) const {
    const_iterator it = m_vocab.find(str);
    if (it == m_vocab.end()) return false;
    *v = it->second;
    return true;
  }

  void clear() { m_vocab.clear(); }

  bool empty() const { return m_vocab.empty(); }

  size_t size() const { return m_vocab.size(); }

  iterator find(const std::string& str) { return m_vocab.find(str); }
  const_iterator find(const std::string& str) const { return m_vocab.find(str); }

  int& operator[](const std::string& str) { return m_vocab[str]; }

  iterator begin() { return m_vocab.begin(); }
  const_iterator begin() const { return m_vocab.begin(); }
  iterator end() { return m_vocab.end(); }
  const_iterator end() const { return m_vocab.end(); }

 private:
  std::map<std::string, int> m_vocab;
};

/** A simple STL-std::map based n-gram counts. Basically, we provide
 * typical accessors and mutaors, but we intentionally does not allow
 * erasing elements.
 */
class NgramCounts {
 public:
  // Used to construct the ngram map
  struct NgramComparator {
    bool operator()(const std::vector<int>& a, const std::vector<int>& b) const {
      size_t i;
      const size_t as = a.size();
      const size_t bs = b.size();
      for (i = 0; i < as && i < bs; ++i) {
        if (a[i] < b[i]) {
          return true;
        }
        if (a[i] > b[i]) {
          return false;
        }
      }
      // entries are equal, shortest wins
      return as < bs;
    }
  };

  typedef std::vector<int> Key;
  typedef int Value;
  typedef std::map<Key, Value, NgramComparator>::iterator iterator;
  typedef std::map<Key, Value, NgramComparator>::const_iterator const_iterator;

  NgramCounts() : kDefaultCount(1) { }
  virtual ~NgramCounts() { }

  /**
   * If the specified "ngram" is found, we add counts.
   * If not, we insert the default count in the container. */
  void Add(const Key& ngram) {
    const_iterator it = find(ngram);
    if (it != end()) {
      m_counts[ngram] = it->second + 1;
    } else {
      m_counts[ngram] = kDefaultCount;
    }
  }

  /**
   * Return true iff the specified "ngram" is found in the container.
   */
  bool Lookup(const Key& ngram, Value* v) const {
    const_iterator it = m_counts.find(ngram);
    if (it == m_counts.end()) return false;
    *v = it->second;
    return true;
  }

  /**
   * Clear all elments in the container.
   */
  void clear() { m_counts.clear(); }

  /**
   * Return true iff the container is empty.
   */
  bool empty() const { return m_counts.empty(); }

  /**
   * Return the the number of elements in the container.
   */
  size_t size() const { return m_counts.size(); }

  size_t max_size() const { return m_counts.max_size(); }

  // Note: This is mainly used by unit tests.
  int get_default_count() const { return kDefaultCount; }

  iterator find(const Key& ngram) { return m_counts.find(ngram); }
  const_iterator find(const Key& ngram) const { return m_counts.find(ngram); }

  Value& operator[](const Key& ngram) { return m_counts[ngram]; }

  iterator begin() { return m_counts.begin(); }
  const_iterator begin() const { return m_counts.begin(); }
  iterator end() { return m_counts.end(); }
  const_iterator end() const { return m_counts.end(); }

 private:
  const int kDefaultCount;
  std::map<Key, Value, NgramComparator> m_counts;
};

#endif  // MERT_NGRAM_H_
