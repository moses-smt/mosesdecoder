/*
 * Word.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include <set>
#include "RuleSymbol.h"

class Word : public RuleSymbol
{
public:
	Word(int pos, const std::string &str);
	virtual ~Word();

	virtual bool IsNonTerm() const
	{ return false; }

	const std::string &GetString() const
	{ return m_str; }

	int GetPos() const
	{ return m_pos; }

	void AddAlignment(const Word *other);

	std::set<int> GetAlignment() const;

	void Output(std::ostream &out) const;
	std::string Debug() const;

protected:
	int m_pos; // original position in sentence, NOT in lattice
	std::string m_str;
	std::set<const Word *> m_alignment;
};

