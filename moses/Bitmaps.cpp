#include "Bitmaps.h"
#include "Util.h"


namespace Moses
{
Bitmaps::Bitmaps(size_t inputSize)
{
	m_initBitmap = new WordsBitmap(inputSize);
	m_coll.insert(m_initBitmap);
}

Bitmaps::~Bitmaps()
{
  RemoveAllInColl(m_coll);
}

const WordsBitmap &Bitmaps::GetBitmap(const WordsBitmap &bm)
{
  Coll::const_iterator iter = m_coll.find(&bm);
  if (iter == m_coll.end()) {
	WordsBitmap *newBM = new WordsBitmap(bm);
    m_coll.insert(newBM);
    return *newBM;
  } else {
	return **iter;
  }
}

const WordsBitmap &Bitmaps::GetBitmap(const WordsBitmap &bm, const WordsRange &range)
{
  WordsBitmap *newBM = new WordsBitmap(bm);
  newBM->SetValue(range, true);

  Coll::const_iterator iter = m_coll.find(newBM);
  if (iter == m_coll.end()) {
    m_coll.insert(newBM);
    return *newBM;
  } else {
	delete newBM;
    return **iter;
  }
}

}

