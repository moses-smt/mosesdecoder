/*
 * Manager.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>

class StaticData;

class Manager {
public:
	Manager(const StaticData &staticData);
	virtual ~Manager();

	size_t GetNumScores() const
	{ return 55; }

protected:
	const StaticData &m_staticData;
};

