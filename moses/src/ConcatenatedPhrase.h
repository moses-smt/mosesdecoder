
#pragma once

#include <vector>
#include "WordsRange.h"

class TargetPhrase;
class TargetPhraseCollection;

/** collection of TargetPhrase pointers, which, when
  * concatenated, makes another target phrase */
class ConcatenatedPhrase
{
protected:
  typedef std::vector<const TargetPhrase*> CollType;
  CollType m_coll;

  std::vector<WordsRange> m_sourceRange;

  size_t m_phraseSize;
  mutable TargetPhraseCollection *m_cacheTargetPhraseColl;
public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }

  ConcatenatedPhrase(size_t size)
    : m_coll(size, NULL)
    , m_sourceRange(size)
    , m_phraseSize(0)
    , m_cacheTargetPhraseColl(NULL)
  {}
  ~ConcatenatedPhrase();

  size_t GetSize() const
  { return m_coll.size(); }
  size_t GetPhraseSize() const
  { return m_phraseSize; }

  const WordsRange &GetSourceRange(size_t i) const 
  {return m_sourceRange[i];}
  const TargetPhrase &GetTargetPhrase(size_t i) const 
  {return *m_coll[i];}
  void Set(size_t i
          , const WordsRange &sourceRange
          , const TargetPhrase *phrase);

  //! create actual target phrases from this concatenate phrase
  const TargetPhraseCollection &CreateTargetPhrases() const;
};

typedef std::vector<ConcatenatedPhrase> ConcatenatedPhraseColl;
