/*
 * Word.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <vector>
#include "Word.h"
#include "Util.h"
#include "util/murmur_hash.hh"

using namespace std;

Word::Word() {
	Init<const Moses::Factor*>(m_factors, MAX_NUM_FACTORS, NULL);
}

Word::~Word() {
	// TODO Auto-generated destructor stub
}

void Word::CreateFromString(Moses::FactorCollection &vocab, const std::string &str)
{
	vector<string> toks = Moses::Tokenize(str, "|");
	for (size_t i = 0; i < toks.size(); ++i) {
		const Moses::Factor *factor = vocab.AddFactor(toks[i], false);
		m_factors[i] = factor;
	}
}

size_t Word::hash() const
{
	uint64_t seed = 0;
	size_t ret = util::MurmurHashNative(m_factors, sizeof(Moses::Factor*) * MAX_NUM_FACTORS, seed);
	return ret;
}

bool Word::operator==(const Word &compare) const
{
	int cmp = memcmp(m_factors, compare.m_factors, sizeof(Moses::Factor*) * MAX_NUM_FACTORS);
	return cmp == 0;
}
