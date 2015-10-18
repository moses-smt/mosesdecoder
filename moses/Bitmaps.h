#pragma once

#include <boost/unordered_set.hpp>
#include "WordsBitmap.h"
#include "Util.h"

class Bitmaps
{
	typedef boost::unordered_set<WordsBitmap*, UnorderedComparer<WordsBitmap>, UnorderedComparer<WordsBitmap> > Coll;

public:
	const WordsBitmap &GetBitmap();

};
