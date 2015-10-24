/*
 * Manager.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include <string>
#include "InputPaths.h"
#include "Stack.h"
#include "util/pool.hh"

class StaticData;
class Phrase;

class Manager {
public:
	Manager(const StaticData &staticData, const std::string &inputStr);
	virtual ~Manager();

	const StaticData &GetStaticData() const
	{ return m_staticData; }

	util::Pool &GetPool() const
	{ return m_pool; }

	void Decode();
protected:
	const StaticData &m_staticData;
	Phrase *m_input;
	InputPaths m_inputPaths;
    mutable util::Pool m_pool;

    std::vector<Stack> m_stacks;
};

