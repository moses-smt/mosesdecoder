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
#include <deque>
#include "../ManagerBase.h"
#include "../InputPaths.h"
#include "../Phrase.h"
#include "../MemPool.h"
#include "../Recycler.h"
#include "../EstimatedScores.h"
#include "../legacy/Bitmaps.h"

namespace Moses2
{

class System;
class TranslationTask;
class PhraseImpl;
class SearchNormal;
class Search;
class Hypothesis;
class Sentence;
class OutputCollector;

class Manager : public ManagerBase
{
public:

	Manager(System &sys, const TranslationTask &task, const std::string &inputStr, long translationId);

	virtual ~Manager();

	Recycler<Hypothesis*> &GetHypoRecycle() const
	{ return *m_hypoRecycle; }

	Bitmaps &GetBitmaps()
	{ return *m_bitmaps; }

	const EstimatedScores &GetEstimatedScores() const
	{ return *m_estimatedScores; }

	const InputPaths &GetInputPaths() const
	{ return m_inputPaths; }

	const TargetPhrase &GetInitPhrase() const
	{ return *m_initPhrase; }

	void Decode();

protected:
	mutable Recycler<Hypothesis*> *m_hypoRecycle;

	InputPaths m_inputPaths;
	Bitmaps *m_bitmaps;
	EstimatedScores *m_estimatedScores;
	TargetPhrase *m_initPhrase;

	Search *m_search;

	// must be run in same thread as Decode()
	void Init();
	void CalcFutureScore();
    void OutputBest() const;

};

}

