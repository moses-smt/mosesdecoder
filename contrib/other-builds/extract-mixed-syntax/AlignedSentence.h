/*
 * AlignedSentence.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include <set>
#include "SyntaxTree.h"
#include "ConsistentPhrases.h"
#include "Phrase.h"
#include "moses/TypeDef.h"

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

	const ConsistentPhrases &GetConsistentPhrases() const
	{ return m_consistentPhrases; }

	std::string Debug() const;

protected:
  Phrase m_source, m_target;
  SyntaxTree sourceTree, targetTree;
  ConsistentPhrases m_consistentPhrases;

	void PopulateWordVec(std::vector<Word*> &vec, const std::string &line);
	void PopulateAlignment(const std::string &line);
	std::vector<int> GetSourceAlignmentCount() const;
};


