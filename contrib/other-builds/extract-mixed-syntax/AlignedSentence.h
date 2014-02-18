/*
 * AlignedSentence.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include <vector>
#include <set>
#include "Word.h"
#include "SyntaxTree.h"
#include "ConsistentPhrase.h"
#include "moses/TypeDef.h"

typedef std::vector<Word*> Phrase;

class Parameter;

class AlignedSentence {
public:
	AlignedSentence(const std::string &source,
			const std::string &target,
			const std::string &alignment);
	virtual ~AlignedSentence();
	void CreateConsistentPhrases(const Parameter &params);

	const Phrase &GetPhrase(Moses::FactorDirection direction) const
	{ return (direction == Moses::Input) ? m_source : m_target; }

	const std::vector<ConsistentPhrase> &GetConsistentPhrases() const
	{ return m_consistentPhrases; }

protected:
  Phrase m_source, m_target;
  SyntaxTree sourceTree, targetTree;

  std::vector<ConsistentPhrase> m_consistentPhrases;

	void PopulateWordVec(std::vector<Word*> &vec, const std::string &line);
	void PopulateAlignment(const std::string &line);
	std::vector<int> GetSourceAlignmentCount() const;
};


