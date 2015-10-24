/*
 * Manager.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include "InputPaths.h"

class StaticData;
class Phrase;

class Manager {
public:
	Manager(const StaticData &staticData, const Phrase &input);
	virtual ~Manager();

	const StaticData &GetStaticData() const
	{ return m_staticData; }

protected:
	const StaticData &m_staticData;
	const Phrase &m_input;
	InputPaths m_inputPaths;
};

