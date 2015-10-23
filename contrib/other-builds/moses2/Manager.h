/*
 * Manager.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include "PhraseTable.h"
#include "InputPaths.h"

class StaticData;
class Phrase;

class Manager {
public:
	Manager(const StaticData &staticData, Phrase &input);
	virtual ~Manager();

	size_t GetNumScores() const
	{ return 55; }

protected:
	const StaticData &m_staticData;
	PhraseTable m_pt;
	Phrase &m_input;
	InputPaths m_inputPaths;
};

