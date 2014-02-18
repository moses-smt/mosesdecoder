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
	Word(const std::string &str);
	virtual ~Word();

	virtual bool IsNonTerm() const
	{ return false; }

	const std::string &GetString() const
	{ return m_str; }

	void AddAlignment(int align);
	const std::set<int> &GetAlignment() const
	{ return m_alignment; }
protected:
	std::string m_str;
	std::set<int> m_alignment;
	int m_highestAlignment, m_lowestAlignment;
};

