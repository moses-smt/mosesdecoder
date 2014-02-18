/*
 * AlignedSentence.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */

#include "moses/Util.h"
#include "AlignedSentence.h"
#include "Parameter.h"

using namespace std;

AlignedSentence::AlignedSentence(const std::string &source,
			const std::string &target,
			const std::string &alignment)
{
	PopulateWordVec(m_source, source);
	PopulateWordVec(m_target, target);
	PopulateAlignment(alignment);

}

AlignedSentence::~AlignedSentence() {
	// TODO Auto-generated destructor stub
}

void AlignedSentence::PopulateWordVec(std::vector<Word*> &vec, const std::string &line)
{
	std::vector<string> toks;
	Moses::Tokenize(toks, line);

	vec.resize(toks.size());
	for (size_t i = 0; i < vec.size(); ++i) {
		const string &tok = toks[i];
		Word *word = new Word(tok);
		vec[i] = word;
	}
}

void AlignedSentence::PopulateAlignment(const std::string &line)
{
	vector<string> alignStr;
	Moses::Tokenize(alignStr, line);

	for (size_t i = 0; i < alignStr.size(); ++i) {
		vector<int> alignPair;
		Moses::Tokenize(alignPair, alignStr[i], "-");
		assert(alignPair.size() == 2);

		int sourcePos = alignPair[0];
		int targetPos = alignPair[1];

		Word *sourceWord = m_source[sourcePos];
		sourceWord->AddAlignment(targetPos);

		Word *targetWord = m_target[targetPos];
		targetWord->AddAlignment(sourcePos);
	}
}

void AlignedSentence::CreateTunnels(const Parameter &params)
{

}
