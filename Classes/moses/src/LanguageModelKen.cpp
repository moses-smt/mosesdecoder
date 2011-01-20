// $Id: LanguageModelKen.cpp 3768 2010-12-09 22:13:09Z heafield $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <cassert>
#include <cstring>
#include <iostream>
#include "lm/binary_format.hh"
#include "lm/enumerate_vocab.hh"
#include "lm/model.hh"

#include "LanguageModelKen.h"
#include "FFState.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "InputFileStream.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{

namespace {

class MappingBuilder : public lm::ngram::EnumerateVocab {
	public:
		MappingBuilder(FactorType factorType, FactorCollection &factorCollection, std::vector<lm::WordIndex> &mapping) 
		: m_factorType(factorType), m_factorCollection(factorCollection), m_mapping(mapping) {}

		void Add(lm::WordIndex index, const StringPiece &str) {
			m_word.assign(str.data(), str.size());
			std::size_t factorId = m_factorCollection.AddFactor(Output, m_factorType, m_word)->GetId();
			if (m_mapping.size() <= factorId) {
				// 0 is <unk> :-)
				m_mapping.resize(factorId + 1);
			}
			m_mapping[factorId] = index;
		}

	private:
		std::string m_word;
		FactorType m_factorType;
		FactorCollection &m_factorCollection;
	std::vector<lm::WordIndex> &m_mapping;
};

struct KenLMState : public FFState {
	lm::ngram::State state;
	int Compare(const FFState &o) const {
		const KenLMState &other = static_cast<const KenLMState &>(o);
		if (state.valid_length_ < other.state.valid_length_) return -1;
		if (state.valid_length_ > other.state.valid_length_) return 1;
		return std::memcmp(state.history_, other.state.history_, sizeof(lm::WordIndex) * state.valid_length_);
	}
};

/** Implementation of single factor LM using Ken's code.
*/
template <class Model> class LanguageModelKen : public LanguageModelSingleFactor
{
private:
	Model *m_ngram;
	std::vector<lm::WordIndex> m_lmIdLookup;
	bool m_lazy;
	FFState *m_nullContextState;
	FFState *m_beginSentenceState;

	void TranslateIDs(const std::vector<const Word*> &contextFactor, lm::WordIndex *indices) const;
	
public:
	LanguageModelKen(bool lazy);
	~LanguageModelKen();

	bool Load(const std::string &filePath
					, FactorType factorType
					, size_t nGramOrder);

	float GetValueGivenState(const std::vector<const Word*> &contextFactor, FFState &state, unsigned int* len = 0) const;
	float GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &outState, unsigned int* len=0) const;
	void GetState(const std::vector<const Word*> &contextFactor, FFState &outState) const;

	FFState *GetNullContextState() const;
	FFState *GetBeginSentenceState() const;
	FFState *NewState(const FFState *from = NULL) const;

	lm::WordIndex GetLmID(const std::string &str) const;

	void CleanUpAfterSentenceProcessing() {}
	void InitializeBeforeSentenceProcessing() {}

};

template <class Model> void LanguageModelKen<Model>::TranslateIDs(const std::vector<const Word*> &contextFactor, lm::WordIndex *indices) const
{
	FactorType factorType = GetFactorType();
	// set up context
	for (size_t i = 0 ; i < contextFactor.size(); i++)
	{
		std::size_t factor = contextFactor[i]->GetFactor(factorType)->GetId();
		lm::WordIndex new_word = (factor >= m_lmIdLookup.size() ? 0 : m_lmIdLookup[factor]);
		indices[contextFactor.size() - 1 - i] = new_word;
	}
}

template <class Model> LanguageModelKen<Model>::LanguageModelKen(bool lazy)
:m_ngram(NULL), m_lazy(lazy)
{
}

template <class Model> LanguageModelKen<Model>::~LanguageModelKen()
{
	delete m_ngram;
}

template <class Model> bool LanguageModelKen<Model>::Load(const std::string &filePath, 
			     FactorType factorType, 
			     size_t /*nGramOrder*/)
{
	m_factorType  = factorType;
	m_filePath    = filePath;

	FactorCollection &factorCollection = FactorCollection::Instance();
	m_sentenceStart = factorCollection.AddFactor(Output, m_factorType, BOS_);
	m_sentenceStartArray[m_factorType] = m_sentenceStart;
	m_sentenceEnd = factorCollection.AddFactor(Output, m_factorType, EOS_);
	m_sentenceEndArray[m_factorType] = m_sentenceEnd;

	MappingBuilder builder(m_factorType, factorCollection, m_lmIdLookup);
	lm::ngram::Config config;

	IFVERBOSE(1) {
		config.messages = &std::cerr;
	} else {
		config.messages = NULL;
	}

	config.enumerate_vocab = &builder;
	config.load_method = m_lazy ? util::LAZY : util::POPULATE_OR_READ;

	m_ngram = new Model(filePath.c_str(), config);
	m_nGramOrder  = m_ngram->Order();

	KenLMState *tmp = new KenLMState();
	tmp->state = m_ngram->NullContextState();
	m_nullContextState = tmp;
	tmp = new KenLMState();
	tmp->state = m_ngram->BeginSentenceState();
	m_beginSentenceState = tmp;
	return true;
}

template <class Model> float LanguageModelKen<Model>::GetValueGivenState(const std::vector<const Word*> &contextFactor, FFState &state, unsigned int* len) const
{
	if (contextFactor.empty())
	{
		return 0;
	}
	lm::ngram::State &realState = static_cast<KenLMState&>(state).state;
	std::size_t factor = contextFactor.back()->GetFactor(GetFactorType())->GetId();
	lm::WordIndex new_word = (factor >= m_lmIdLookup.size() ? 0 : m_lmIdLookup[factor]);
	lm::ngram::State copied(realState);
	lm::FullScoreReturn ret(m_ngram->FullScore(copied, new_word, realState));

	if (len)
	{
		*len = ret.ngram_length;
	}
	return TransformLMScore(ret.prob);
}

template <class Model> float LanguageModelKen<Model>::GetValueForgotState(const vector<const Word*> &contextFactor, FFState &outState, unsigned int* len) const
{
	if (contextFactor.empty())
	{
		static_cast<KenLMState&>(outState).state = m_ngram->NullContextState();
		return 0;
	}
	
	lm::WordIndex indices[contextFactor.size()];
	TranslateIDs(contextFactor, indices);

	lm::FullScoreReturn ret(m_ngram->FullScoreForgotState(indices + 1, indices + contextFactor.size(), indices[0], static_cast<KenLMState&>(outState).state));
	if (len)
	{
		*len = ret.ngram_length;
	}
	return TransformLMScore(ret.prob);
}

template <class Model> void LanguageModelKen<Model>::GetState(const std::vector<const Word*> &contextFactor, FFState &outState) const {
	if (contextFactor.empty()) {
		static_cast<KenLMState&>(outState).state = m_ngram->NullContextState();
		return;
	}
	lm::WordIndex indices[contextFactor.size()];
	TranslateIDs(contextFactor, indices);
	m_ngram->GetState(indices, indices + contextFactor.size(), static_cast<KenLMState&>(outState).state);
}

template <class Model> FFState *LanguageModelKen<Model>::GetNullContextState() const {
	return m_nullContextState;
}

template <class Model> FFState *LanguageModelKen<Model>::GetBeginSentenceState() const {
	return m_beginSentenceState;
}

template <class Model> FFState *LanguageModelKen<Model>::NewState(const FFState *from) const {
	KenLMState *ret = new KenLMState;
	if (from) {
		ret->state = static_cast<const KenLMState&>(*from).state;
	}
	return ret;
}

template <class Model> lm::WordIndex LanguageModelKen<Model>::GetLmID(const std::string &str) const {
	return m_ngram->GetVocabulary().Index(str);
}

} // namespace

LanguageModelSingleFactor *ConstructKenLM(const std::string &file, bool lazy) {
	lm::ngram::ModelType model_type;
	if (lm::ngram::RecognizeBinary(file.c_str(), model_type)) {
		switch(model_type) {
			case lm::ngram::HASH_PROBING:
				return new LanguageModelKen<lm::ngram::ProbingModel>(lazy);
			case lm::ngram::HASH_SORTED:
				return new LanguageModelKen<lm::ngram::SortedModel>(lazy);
			case lm::ngram::TRIE_SORTED:
				return new LanguageModelKen<lm::ngram::TrieModel>(lazy);
			default:
				std::cerr << "Unrecognized kenlm model type " << model_type << std::endl;
				abort();
		}
	} else {
		return new LanguageModelKen<lm::ngram::ProbingModel>(lazy);
	}
}

}

