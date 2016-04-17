/*
 * TargetPhrases.h
 *
 *  Created on: 15 Apr 2016
 *      Author: hieu
 */

#pragma once
#include <vector>
#include <stddef.h>

namespace Moses2
{
namespace SCFG
{
class TargetPhraseImpl;

class TargetPhrases
{
  typedef std::vector<const SCFG::TargetPhraseImpl*> Coll;

public:
  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;
  //! iterators
  const_iterator begin() const
  {
    return m_coll.begin();
  }
  const_iterator end() const
  {
    return m_coll.end();
  }

  TargetPhrases();
  virtual ~TargetPhrases();

  size_t GetSize() const
  { return m_coll.size(); }

  void AddTargetPhrase(const SCFG::TargetPhraseImpl &targetPhrase)
  {
    m_coll.push_back(&targetPhrase);
  }

protected:
  Coll m_coll;

};

}
} /* namespace Moses2 */

