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
#include <boost/pool/object_pool.hpp>
#include "../InputPaths.h"
#include "../TargetPhrase.h"
#include "../MemPool.h"
#include "../Recycler.h"
#include "../legacy/Bitmaps.h"
#include "../legacy/SquareMatrix.h"

class System;
class TranslationTask;
class PhraseImpl;
class SearchNormal;
class Search;
class Hypothesis;

class Manager {
public:
	const System &system;
	const TranslationTask &task;

	Manager(System &sys, const TranslationTask &task, const std::string &inputStr);

	virtual ~Manager();

	MemPool &GetPool() const
	{ return *m_pool; }

	boost::object_pool<Hypothesis> &GetHypoPool() const
	{ return *m_hypoPool; }

	Bitmaps &GetBitmaps()
	{ return *m_bitmaps; }

	const PhraseImpl &GetInput() const
	{ return *m_input; }

	const SquareMatrix &GetEstimatedScores() const
	{ return *m_estimatedScores; }

	const InputPaths &GetInputPaths() const
	{ return m_inputPaths; }

	const TargetPhrase &GetInitPhrase() const
	{ return *m_initPhrase; }

	const Range &GetInitRange() const
	{ return m_initRange; }

	void Decode();

    void OutputBest(std::ostream &out) const;

protected:
	mutable MemPool *m_pool;
	mutable boost::object_pool<Hypothesis> *m_hypoPool;

    std::string m_inputStr;
	PhraseImpl *m_input;
	InputPaths m_inputPaths;
	Bitmaps *m_bitmaps;
	SquareMatrix *m_estimatedScores;
	Range m_initRange;
	TargetPhrase *m_initPhrase;

	Search *m_search;

	// must be run in same thread as Decode()
	void Init();

	void CalcFutureScore();
};

