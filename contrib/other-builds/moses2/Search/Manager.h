/*
 * Manager.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <queue>
#include <cstddef>
#include <string>
#include <vector>
#include "../InputPaths.h"
#include "../TargetPhrase.h"
#include "../MemPool.h"
#include "../Recycler.h"
#include "Stack.h"
#include "moses/Bitmaps.h"
#include "moses/SquareMatrix.h"

class System;
class PhraseImpl;
class SearchNormal;

class Manager {
public:
	Manager(System &system, const std::string &inputStr);
	virtual ~Manager();

	MemPool &GetPool() const
	{ return *m_pool; }

	Recycler<Hypothesis*> &GetHypoRecycle() const
	{ return *m_hypoRecycle; }

	const System &GetSystem() const
	{ return m_system; }

	Moses::Bitmaps &GetBitmaps()
	{ return *m_bitmaps; }

	const PhraseImpl &GetInput() const
	{ return *m_input; }

	const InputPaths &GetInputPaths() const
	{ return m_inputPaths; }

	const Hypothesis *GetBestHypothesis() const;

	void Decode();
protected:
	mutable MemPool *m_pool;
    mutable Recycler<Hypothesis*> *m_hypoRecycle;

	const System &m_system;
	PhraseImpl *m_input;
	InputPaths m_inputPaths;
	Moses::Bitmaps *m_bitmaps;
	Moses::SquareMatrix *m_futureScore;
	Moses::Range m_initRange;
	TargetPhrase m_initPhrase;

    std::vector<Stack> m_stacks;
	SearchNormal *m_search;

	void CalcFutureScore();
};

