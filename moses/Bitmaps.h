#pragma once

#include <boost/unordered_set.hpp>
#include "WordsBitmap.h"
#include "Util.h"

namespace Moses
{

class Bitmaps
{
	typedef boost::unordered_set<const WordsBitmap*, UnorderedComparer<WordsBitmap>, UnorderedComparer<WordsBitmap> > Coll;
	Coll m_coll;

public:
	virtual ~Bitmaps();
	const WordsBitmap &GetBitmap(const WordsBitmap &bm);

};

}
