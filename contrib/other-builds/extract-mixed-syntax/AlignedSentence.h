/*
 * AlignedSentence.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include <set>
#include "Word.h"
#include "SyntaxTree.h"
#include "ConsistentPhrases.h"
#include "moses/TypeDef.h"

class Parameter;

class Phrase : public std::vector<Word*>
{
public:
	void Debug(std::ostream &out) const;

};

class AlignedSentence {
public:
	AlignedSentence(const std::string &source,
			const std::string &target,
			const std::string &alignment);
	virtual ~AlignedSentence();
	void CreateConsistentPhrases(const Parameter &params);

	const Phrase &GetPhrase(Moses::FactorDirection direction) const
	{ return (direction == Moses::Input) ? m_source : m_target; }

	void Debug(std::ostream &out) const;

protected:
  Phrase m_source, m_target;
  SyntaxTree sourceTree, targetTree;
  ConsistentPhrases m_consistentPhrases;

	void PopulateWordVec(std::vector<Word*> &vec, const std::string &line);
	void PopulateAlignment(const std::string &line);
	std::vector<int> GetSourceAlignmentCount() const;
};


