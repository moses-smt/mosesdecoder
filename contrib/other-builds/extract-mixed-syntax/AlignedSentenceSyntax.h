/*
 * AlignedSentenceSyntax.h
 *
 *  Created on: 26 Feb 2014
 *      Author: hieu
 */

#pragma once

#include "AlignedSentence.h"
#include "SyntaxTree.h"

class AlignedSentenceSyntax : public AlignedSentence
{
public:
	AlignedSentenceSyntax(const std::string &source,
			const std::string &target,
			const std::string &alignment);
	virtual ~AlignedSentenceSyntax();

	void CreateConsistentPhrases(const Parameter &params);

protected:
	std::string m_source, m_target, m_alignment;
	SyntaxTree sourceTree, targetTree;

};

