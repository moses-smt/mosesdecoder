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

class StaticData;
class Phrase;
class SearchNormal;

class Manager {
public:
	Manager(const StaticData &staticData, const std::string &inputStr);
	virtual ~Manager();

	util::Pool &GetPool() const
	{ return m_pool; }

	const StaticData &GetStaticData() const
	{ return m_staticData; }

	const Moses::Bitmaps &GetBitmaps() const
	{ return *m_bitmaps; }

	const InputPaths &GetInputPaths() const
	{ return m_inputPaths; }

	void Decode();
protected:
	mutable util::Pool m_pool;

	const StaticData &m_staticData;
	Phrase *m_input;
	InputPaths m_inputPaths;
	Moses::Bitmaps *m_bitmaps;
	Moses::Range m_initRange;

    std::vector<Stack> m_stacks;
	SearchNormal *m_search;
};

