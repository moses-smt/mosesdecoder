#pragma once
/*
 *  SymbolSequence.h
 *  extract
 *
 *  Created by Hieu Hoang on 21/07/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <iostream>
#include <vector>
#include "Symbol.h"

class SymbolSequence
{
	friend std::ostream& operator<<(std::ostream &out, const SymbolSequence &obj);

protected:
	typedef std::vector<Symbol> CollType;
	CollType m_coll;
	
public:
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	const_iterator begin() const { return m_coll.begin(); }
	const_iterator end() const { return m_coll.end(); }
	
	void Add(const Symbol &symbol)
	{
		m_coll.push_back(symbol);
	}
	size_t GetSize() const
	{ return m_coll.size(); }
	const Symbol &GetSymbol(size_t ind) const
	{ return m_coll[ind]; }

	void Clear()
	{ m_coll.clear(); }
	
	int Compare(const SymbolSequence &other) const;

};
