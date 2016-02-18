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
#include "../InputPaths.h"
#include "../TargetPhrase.h"
#include "../MemPool.h"
#include "../Recycler.h"
#include "../EstimatedScores.h"
#include "../legacy/Bitmaps.h"
#include "lm/state.hh"
#include "lm/word_index.hh"

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

class Manager {
public:
	const System &system;
	const TranslationTask &task;

	Manager(System &sys, const TranslationTask &task, const std::string &inputStr, long translationId);

	virtual ~Manager();

	MemPool &GetPool() const
	{ return *m_pool; }

	MemPool &GetSystemPool() const
	{ return *m_systemPool; }

	Recycler<Hypothesis*> &GetHypoRecycle() const
	{ return *m_hypoRecycle; }

	Bitmaps &GetBitmaps()
	{ return *m_bitmaps; }

	const Sentence &GetInput() const
	{ return *m_input; }

	const EstimatedScores &GetEstimatedScores() const
	{ return *m_estimatedScores; }

	const InputPaths &GetInputPaths() const
	{ return m_inputPaths; }

	const TargetPhrase &GetInitPhrase() const
	{ return *m_initPhrase; }

	void Decode();

    void OutputBest() const;

    //void AddLMCache(const lm::ngram::State &in_state, const lm::WordIndex new_word) const;
    bool FindLMCache(const lm::ngram::State &in_state, const lm::WordIndex new_word, float &score, lm::ngram::State &out_state) const;
    void AddLMCache(const lm::ngram::State &in_state, const lm::WordIndex new_word, float score, const lm::ngram::State &out_state) const;

protected:
	mutable MemPool *m_pool, *m_systemPool;
	mutable Recycler<Hypothesis*> *m_hypoRecycle;

    std::string m_inputStr;
    long m_translationId;
	Sentence *m_input;
	InputPaths m_inputPaths;
	Bitmaps *m_bitmaps;
	EstimatedScores *m_estimatedScores;
	TargetPhrase *m_initPhrase;

	Search *m_search;

	typedef std::pair<lm::ngram::State, lm::WordIndex> LMCacheKey;
	typedef std::pair<float, lm::ngram::State> LMCacheValue;
	mutable boost::unordered_map<LMCacheKey, LMCacheValue> m_lmCache;

	// must be run in same thread as Decode()
	void Init();

	void CalcFutureScore();
};

}

