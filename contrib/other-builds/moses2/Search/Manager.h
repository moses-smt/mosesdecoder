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
#include "../InputPaths.h"
#include "../TargetPhrase.h"
#include "../MemPool.h"
#include "../Recycler.h"
#include "Stacks.h"
#include "../legacy/Bitmaps.h"
#include "../legacy/SquareMatrix.h"

class System;
class PhraseImpl;
class SearchNormal;
class Search;

class Manager {
public:
	const System &system;

	Manager(System &sys, const std::string &inputStr)
	:system(sys)
	,m_inputStr(inputStr)
	,m_initRange(NOT_FOUND, NOT_FOUND)
	{}

	virtual ~Manager();

	MemPool &GetPool() const
	{ return *m_pool; }

	Recycler<Hypothesis*> &GetHypoRecycle() const
	{ return m_hypoRecycle; }

	Bitmaps &GetBitmaps()
	{ return *m_bitmaps; }

	const PhraseImpl &GetInput() const
	{ return *m_input; }

	const SquareMatrix &GetEstimatedScores() const
	{ return *m_estimatedScores; }

	const InputPaths &GetInputPaths() const
	{ return m_inputPaths; }

	const Hypothesis *GetBestHypothesis() const;

	void Decode();
protected:
	mutable MemPool *m_pool;
    mutable Recycler<Hypothesis*> m_hypoRecycle;

    std::string m_inputStr;
	PhraseImpl *m_input;
	InputPaths m_inputPaths;
	Bitmaps *m_bitmaps;
	SquareMatrix *m_estimatedScores;
	Range m_initRange;
	TargetPhrase *m_initPhrase;

    Stacks m_stacks;
	Search *m_search;

	// must be run in same thread as Decode()
	void Init();

	void CalcFutureScore();
};

