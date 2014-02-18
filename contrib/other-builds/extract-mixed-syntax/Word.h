/*
 * Word.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include <set>

class Word {
public:
	Word(const std::string &str);
	virtual ~Word();

	void AddAlignment(int align);
	const std::set<int> &GetAlignment() const
	{ return m_alignment; }
protected:
	std::set<int> m_alignment;
	int m_highestAlignment, m_lowestAlignment;
};

