/*
 * Word.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "Word.h"
#include "Util.h"
#include "util/murmur_hash.hh"

Word::Word() {
	Init<Factor*>(m_factors, MAX_NUM_FACTORS, NULL);
}

Word::~Word() {
	// TODO Auto-generated destructor stub
}

size_t Word::hash() const
{
	uint64_t seed = 0;
	size_t ret = util::MurmurHashNative(m_factors, sizeof(Factor*) * MAX_NUM_FACTORS, seed);
	return ret;
}

bool Word::operator==(const Word &compare) const
{
	int cmp = memcmp(m_factors, compare.m_factors, sizeof(Factor*) * MAX_NUM_FACTORS);
	return cmp == 0;
}
