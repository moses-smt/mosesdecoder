#include "Bitmaps.h"
#include "Util.h"


namespace Moses
{
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
	}
	else {
		return **iter;
	}
}

}

