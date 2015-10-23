#pragma once

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <set>
#include "WordsBitmap.h"
#include "Util.h"

namespace Moses
{

class Bitmaps
{
  typedef boost::unordered_map<WordsRange, const WordsBitmap*> NextBitmaps;
  typedef boost::unordered_map<const WordsBitmap*, NextBitmaps, UnorderedComparer<WordsBitmap>, UnorderedComparer<WordsBitmap> > Coll;
  //typedef std::set<const WordsBitmap*, OrderedComparer<WordsBitmap> > Coll;
  Coll m_coll;
  const WordsBitmap *m_initBitmap;

  const WordsBitmap &GetNextBitmap(const WordsBitmap &bm, const WordsRange &range);
public:
  Bitmaps(size_t inputSize, const std::vector<bool> &initSourceCompleted);
  virtual ~Bitmaps();

  const WordsBitmap &GetInitialBitmap() const {
    return *m_initBitmap;
  }
  const WordsBitmap &GetBitmap(const WordsBitmap &bm, const WordsRange &range);

};

}
