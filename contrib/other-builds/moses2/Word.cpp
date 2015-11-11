/*
 * Word.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include <vector>
#include "Word.h"
#include "legacy/Util2.h"
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
		const string &tok = toks[i];
		//cerr << "tok=" << tok << endl;
		const Moses::Factor *factor = vocab.AddFactor(tok, false);
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

std::ostream& operator<<(std::ostream &out, const Word &obj)
{
	bool outputAlready = false;
	for (size_t i = 0; i < MAX_NUM_FACTORS; ++i) {
		const Moses::Factor *factor = obj.m_factors[i];
		if (factor) {
			if (outputAlready) {
				out << "|";
			}
			out << *factor;
			outputAlready = true;
		}
	}
	return out;
}
