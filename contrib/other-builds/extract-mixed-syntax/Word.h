/*
 * Word.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include <set>
#include "LatticeArc.h"

class Word : public LatticeArc
{
public:
	Word(int pos, const std::string &str);
	virtual ~Word();

	virtual bool IsNonTerm() const
	{ return false; }

	const std::string &GetString() const
	{ return m_str; }

	int GetStart() const
	{ return m_pos; }

	int GetEnd() const
	{ return m_pos; }

	void AddAlignment(int align);

	const std::set<int> &GetAlignment() const
	{ return m_alignment; }

	int GetHighestAlignment() const
	{ return m_highestAlignment; }

	int GetLowestAlignment() const
	{ return m_lowestAlignment; }

protected:
	int m_pos;
	std::string m_str;
	std::set<int> m_alignment;
	int m_highestAlignment, m_lowestAlignment;
};

