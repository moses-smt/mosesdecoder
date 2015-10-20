#include <boost/foreach.hpp>
#include "Bitmaps.h"
#include "Util.h"

using namespace std;

namespace Moses
{
Bitmaps::Bitmaps(size_t inputSize, const std::vector<bool> &initSourceCompleted)
{
  m_initBitmap = new WordsBitmap(inputSize, initSourceCompleted);
  m_coll[m_initBitmap];
}

Bitmaps::~Bitmaps()
{
  BOOST_FOREACH (const Coll::value_type& myPair, m_coll) {
    const WordsBitmap *bm = myPair.first;
    delete bm;
  }
}

const WordsBitmap &Bitmaps::GetNextBitmap(const WordsBitmap &bm, const WordsRange &range)
{
  WordsBitmap *newBM = new WordsBitmap(bm);
  newBM->SetValue(range, true);

  Coll::const_iterator iter = m_coll.find(newBM);
  if (iter == m_coll.end()) {
    m_coll[newBM] = NextBitmaps();
    return *newBM;
  } else {
    delete newBM;
    return *iter->first;
  }
}

const WordsBitmap &Bitmaps::GetBitmap(const WordsBitmap &bm, const WordsRange &range)
{
  Coll::iterator iter = m_coll.find(&bm);
  assert(iter != m_coll.end());

  const WordsBitmap *newBM;
  NextBitmaps &next = iter->second;
  NextBitmaps::const_iterator iterNext = next.find(range);
  if (iterNext == next.end()) {
    // not seen the link yet.
    newBM = &GetNextBitmap(bm, range);
    next[range] = newBM;
  } else {
    // link exist
    //std::cerr << "link exists" << endl;
    newBM = iterNext->second;
  }
  return *newBM;
}

}

