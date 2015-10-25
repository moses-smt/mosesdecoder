/*
 * Manager.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "InputPaths.h"
#include "Stack.h"
#include "moses/Bitmaps.h"
#include "util/pool.hh"

class System;
class Phrase;
class SearchNormal;

class Manager {
public:
	Manager(const System &system, const std::string &inputStr);
	virtual ~Manager();

	util::Pool &GetPool()
	{ return m_pool; }

	const System &GetStaticData() const
	{ return m_staticData; }

	Moses::Bitmaps &GetBitmaps()
	{ return *m_bitmaps; }

	const InputPaths &GetInputPaths() const
	{ return m_inputPaths; }

	void Decode();
protected:
	util::Pool m_pool;

	const System &m_staticData;
	Phrase *m_input;
	InputPaths m_inputPaths;
	Moses::Bitmaps *m_bitmaps;
	Moses::Range m_initRange;

    std::vector<Stack> m_stacks;
	SearchNormal *m_search;
};

