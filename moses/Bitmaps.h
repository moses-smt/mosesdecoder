#pragma once

#include <boost/unordered_set.hpp>
#include <set>
#include "WordsBitmap.h"
#include "Util.h"

namespace Moses
{

class Bitmaps
{
  typedef boost::unordered_set<WordsRange, const WordsBitmap*> NextBitmaps;
  typedef boost::unordered_set<const WordsBitmap*, UnorderedComparer<WordsBitmap>, UnorderedComparer<WordsBitmap> > Coll;
  //typedef std::set<const WordsBitmap*, OrderedComparer<WordsBitmap> > Coll;
  Coll m_coll;
  const WordsBitmap *m_initBitmap;
public:
  Bitmaps(size_t inputSize);
  virtual ~Bitmaps();

  const WordsBitmap &GetInitialBitmap() const
  { return *m_initBitmap; }
  const WordsBitmap &GetBitmap(const WordsBitmap &bm);
  const WordsBitmap &GetBitmap(const WordsBitmap &bm, const WordsRange &range);

};

}
