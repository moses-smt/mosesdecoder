#pragma once

#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <set>
#include "Bitmap.h"
#include "Util.h"

namespace Moses
{

class Bitmaps
{
  typedef boost::unordered_map<Range, const Bitmap*> NextBitmaps;
  typedef boost::unordered_map<const Bitmap*, NextBitmaps, UnorderedComparer<Bitmap>, UnorderedComparer<Bitmap> > Coll;
  //typedef std::set<const Bitmap*, OrderedComparer<Bitmap> > Coll;
  Coll m_coll;
  const Bitmap *m_initBitmap;

  const Bitmap &GetNextBitmap(const Bitmap &bm, const Range &range);
public:
  Bitmaps(size_t inputSize, const std::vector<bool> &initSourceCompleted);
  virtual ~Bitmaps();

  const Bitmap &GetInitialBitmap() const {
    return *m_initBitmap;
  }
  const Bitmap &GetBitmap(const Bitmap &bm, const Range &range);

};

}
