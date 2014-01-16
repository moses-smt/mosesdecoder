#pragma once

/*
 *  Symbol.h
 *  extract
 *
 *  Created by Hieu Hoang on 21/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include <iostream>
#include <vector>

class Symbol
{
	friend std::ostream& operator<<(std::ostream &out, const Symbol &obj);

protected:
	std::string m_label, m_labelT; // m_labelT only for non-term
	std::vector<std::pair<size_t, size_t> > m_span;
	
	bool m_isTerminal, m_isSourceSyntax, m_isTargetSyntax;
public:
	// for terminals
	Symbol(const std::string &label, size_t pos);

	// for non-terminals
	Symbol(const std::string &labelS, const std::string &labelT
				 , size_t startS, size_t endS
				 , size_t startT, size_t endT
				 , bool isSourceSyntax, bool isTargetSyntax);

	int Compare(const Symbol &other) const;

};
