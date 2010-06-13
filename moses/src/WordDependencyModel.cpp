#include <cassert>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "FFState.h"
#include "Hypothesis.h"
#include "LanguageModel.h"
#include "Word.h"
#include "WordDependencyModel.h"
#include "TypeDef.h"
#include "Util.h"

namespace Moses
{

class WordDependencyState : public FFState
{
private:
	typedef std::map<size_t, std::pair<std::string,std::string> > LinkMap;
	
	LinkMap m_openLinks;
	const WordDependencyModel *m_model;

public:
	WordDependencyState(const WordDependencyModel	*model) : m_model(model) {}

	WordDependencyState *Clone() const;
	const Scores ProcessAntecedent(size_t linkNo, const std::string &word);
	const Scores ProcessReferent(size_t linkNo, const std::string &word);
	virtual int Compare(const FFState &other) const;
};

const FFState* WordDependencyModel::Evaluate(const Hypothesis& cur_hypo,
														const FFState* prev_state,
														ScoreComponentCollection* accumulator) const
{
	const WordDependencyState *state = dynamic_cast<const WordDependencyState *>(prev_state);
	WordDependencyState *new_state = NULL;
	
	for(size_t i = 0; i < cur_hypo.GetCurrTargetLength(); i++)
	{
		const std::string &link = cur_hypo.GetSourcePhrase()->GetFactor(i, m_linkFactor)->GetString();
		
		std::vector<std::string> linkPart = Tokenize(link, "-");
		assert(linkPart.size() == 2);
		
		if(linkPart[0] != "*")
		{
			if(!new_state)
				new_state = state->Clone();
			
			size_t linkNo = Scan<size_t>(linkPart[0]);
			const std::string word = cur_hypo.GetTargetPhraseStringRep(m_eFactors);
			accumulator->PlusEquals(this, new_state->ProcessAntecedent(linkNo, word));
		}

		if(linkPart[1] != "*")
		{
			if(!new_state)
				new_state = state->Clone();
			
			size_t linkNo = Scan<size_t>(linkPart[1]);
			const std::string word = cur_hypo.GetTargetPhraseStringRep(m_eFactors);
			accumulator->PlusEquals(this, new_state->ProcessReferent(linkNo, word));
		}
	}
	
	return new_state ? new_state : prev_state;
}

const FFState* WordDependencyModel::EmptyHypothesisState(const InputType &input) const
{
	return new WordDependencyState(this);
}

const Scores WordDependencyModel::LookupScores(const std::string &antecedent, const std::string &referent) const
{
	Scores s(1);
	Word antw, refw;
	std::vector<const Word *> bigram(2);
	
	antw.CreateFromString(Output, m_eFactors, antecedent, false);
	refw.CreateFromString(Output, m_eFactors, referent, false);
	bigram[0] = &antw;
	bigram[1] = &refw;
	s[0] = m_lm->GetValue(bigram);
	
	return s;
}

WordDependencyState *WordDependencyState::Clone() const
{
	return new WordDependencyState(*this);
}

const Scores WordDependencyState::ProcessAntecedent(size_t linkNo, const std::string &word)
{
	Scores scores(m_model->GetNumScoreComponents(), .0f);
	
	LinkMap::iterator it = m_openLinks.find(linkNo);
	if(it == m_openLinks.end())
		m_openLinks[linkNo] = std::make_pair(word, "");
	else
	{
		assert(it->second.first == "" && it->second.second != "");
		scores = m_model->LookupScores(word, it->second.second);
		m_openLinks.erase(it);
	}
	
	return scores;
}

const Scores WordDependencyState::ProcessReferent(size_t linkNo, const std::string &word)
{
	Scores scores(m_model->GetNumScoreComponents(), .0f);
	
	LinkMap::iterator it = m_openLinks.find(linkNo);
	if(it == m_openLinks.end())
		m_openLinks[linkNo] = std::make_pair("", word);
	else
	{
		assert(it->second.first != "" && it->second.second == "");
		scores = m_model->LookupScores(it->second.first, word);
		m_openLinks.erase(it);
	}
	
	return scores;
}

int WordDependencyState::Compare(const FFState &other) const
{
	const WordDependencyState &o = dynamic_cast<const WordDependencyState &>(other);
	
	if(&o == this)
		return 0;

	assert(m_model == o.m_model);
	
	if(m_openLinks.size() < o.m_openLinks.size())
		return -1;
	else if(m_openLinks.size() > o.m_openLinks.size())
		return 1;
	
	LinkMap::const_iterator i, j;
	for(i = m_openLinks.begin(), j = o.m_openLinks.begin(); i != m_openLinks.end(); ++i, ++j)
		if(*i < *j)
			return -1;
		else if(*i > *j)
			return 1;
	
	return 0;
}

}
