
#pragma once

#include <vector>

class TargetPhrase;

class TargetPhrases
{
  typedef std::vector<const TargetPhrase*> Coll;

public:
  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;

  const_iterator begin() const {
    return m_coll.begin();
  }
  const_iterator end() const {
    return m_coll.end();
  }

  TargetPhrases();
  virtual ~TargetPhrases();

  void Add(const TargetPhrase *tp) {
    m_coll.push_back(tp);
  }

  size_t GetSize() const {
    return m_coll.size();
  }

protected:
  Coll m_coll;

};

