
#pragma once

#include <vector>
#include <list>
#include "TypeDef.h"

class DecodeStepTranslation;

class DecodeStepCollection
{
	typedef std::vector<DecodeStepTranslation*> CollType;
protected:
	CollType m_stepColl;

public:
	//! iterators
	typedef CollType::iterator iterator;
	typedef CollType::const_iterator const_iterator;
	const_iterator begin() const { return m_stepColl.begin(); }
	const_iterator end() const { return m_stepColl.end(); }
	iterator begin() { return m_stepColl.begin(); }
	iterator end() { return m_stepColl.end(); }

	~DecodeStepCollection();

	const DecodeStepTranslation* operator[](size_t i) const 
	{return m_stepColl[i];}

	size_t GetSize() const
	{return m_stepColl.size();}

	void Add(DecodeStepTranslation *decodeStep)
	{
		m_stepColl.push_back(decodeStep);
	}

	void CalcConflictingOutputFactors();
};

