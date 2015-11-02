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
#include "TargetPhrase.h"
#include "MemPool.h"
#include "moses/Bitmaps.h"
#include "moses/SquareMatrix.h"

class System;
class Phrase;
class SearchNormal;

class Manager {
public:
	Manager(System &system, const std::string &inputStr);
	virtual ~Manager();

	MemPool &GetPool() const
	{ return *m_pool; }

	const System &GetSystem() const
	{ return m_system; }

	Moses::Bitmaps &GetBitmaps()
	{ return *m_bitmaps; }

	const InputPaths &GetInputPaths() const
	{ return m_inputPaths; }

	const Hypothesis *GetBestHypothesis() const;

	void Decode();
protected:
	mutable MemPool *m_pool;

	const System &m_system;
	Phrase *m_input;
	InputPaths m_inputPaths;
	Moses::Bitmaps *m_bitmaps;
	Moses::SquareMatrix *m_futureScore;
	Moses::Range m_initRange;
	TargetPhrase m_initPhrase;

    std::vector<Stack> m_stacks;
	SearchNormal *m_search;

	void CalcFutureScore();
};

