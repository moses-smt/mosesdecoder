// $Id$
#include "Dictionary.h"

Dictionary::Dictionary(size_t noScoreComponent)
	:m_noScoreComponent(noScoreComponent)
	,m_factorsUsed(2)
{
}

Dictionary::~Dictionary() {}

void Dictionary::CleanUp() {}
