// vi:ts=2:
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
	typedef std::map<size_t, std::pair<std::vector<std::string>,std::vector<std::string> > > LinkMap;
	
	LinkMap m_openLinks;
	const WordDependencyModel *m_model;

public:
	WordDependencyState(const WordDependencyModel	*model) : m_model(model) {}

	WordDependencyState *Clone() const;
	const std::vector<std::string> ProcessAntecedent(size_t linkNo, const std::vector<std::string> &word);
	const std::vector<std::string> ProcessReferent(size_t linkNo, const std::vector<std::string> &word);
	virtual int Compare(const FFState &other) const;
};

const FFState* WordDependencyModel::Evaluate(const Hypothesis& cur_hypo,
														const FFState* prev_state,
														ScoreComponentCollection* accumulator) const
{
	const WordDependencyState *state = dynamic_cast<const WordDependencyState *>(prev_state);
	WordDependencyState *new_state = state->Clone();
	
	for(size_t i = 0; i < cur_hypo.GetCurrSourceWordsRange().GetNumWordsCovered(); i++)
	{
		const std::string &link = cur_hypo.GetSourcePhrase()->GetFactor(i, m_linkFactor)->GetString();
		
		size_t del = link.find_first_of('-');
		assert(del != std::string::npos);
		std::string ant = link.substr(0, del);
		std::string ref = link.substr(del + 1, std::string::npos);
		assert(ant != "" && ref != "");
		
		if(ant != "*")
		{
			size_t linkNo = Scan<size_t>(ant);
			const std::vector<std::string> antwords = GetAlignedTargetWords(cur_hypo, i);
			const std::vector<std::string> refwords = new_state->ProcessAntecedent(linkNo, refwords);
			if(antwords.size() > 0)
				accumulator->PlusEquals(this, LookupScores(antwords, refwords));
		}

		if(ref != "*")
		{
			if(ref[0] != '>')
			{
				size_t linkNo = Scan<size_t>(ref);
				const std::vector<std::string> refwords = GetAlignedTargetWords(cur_hypo, i);
				const std::vector<std::string> antwords = new_state->ProcessReferent(linkNo, antwords);
				if(refwords.size() > 0)
					accumulator->PlusEquals(this, LookupScores(antwords, refwords));
			}
			else
			{
				const std::vector<std::string> antwords(1, ref.substr(1));
				const std::vector<std::string> refwords = GetAlignedTargetWords(cur_hypo, i);
				assert(antwords.size() > 0);
				accumulator->PlusEquals(this, LookupScores(antwords, refwords));
			}
		}
	}
	
	return new_state;
}

const FFState* WordDependencyModel::EmptyHypothesisState(const InputType &input) const
{
	return new WordDependencyState(this);
}

const Scores WordDependencyModel::LookupScores(const std::vector<std::string> &antecedent, const std::vector<std::string> &referent) const
{
	Scores s(1);
	Word antw, refw;
	std::vector<const Word *> bigram(2);
	
	bigram[0] = &antw;
	bigram[1] = &refw;

	for(size_t i = 0; i < antecedent.size(); i++) {
		antw.CreateFromString(Output, m_eFactors, antecedent[i], false);
		for(size_t j = 0; j < referent.size(); j++) {
			refw.CreateFromString(Output, m_eFactors, referent[j], false);
			float score = m_lm->GetValue(bigram);
			if(score > s[0])
				s[0] = score;
		}
	}
	
	return s;
}

std::vector<std::string> WordDependencyModel::GetAlignedTargetWords(const Hypothesis &hypo, size_t pos) const
{
	std::vector<std::string> ret;
	const TargetPhrase tp = hypo.GetTargetPhrase();
	for(unsigned i = 0; i < tp.GetSize(); i++)
	{
		std::string alig = tp.GetFactor(i, m_alignmentFactor)->GetString();
		if(alig == "-")
			continue;
		std::vector<unsigned> apos = Tokenize<unsigned>(alig, ",");
		for(unsigned j = 0; j < apos.size(); j++)
			if(j == pos)
				ret.push_back(tp.GetWord(j).GetString(m_eFactors, false));
	}
	return ret;
}

WordDependencyState *WordDependencyState::Clone() const
{
	return new WordDependencyState(*this);
}

const std::vector<std::string> WordDependencyState::ProcessAntecedent(size_t linkNo, const std::vector<std::string> &words)
{
	std::vector<std::string> s;
	
	LinkMap::iterator it = m_openLinks.find(linkNo);
	if(it == m_openLinks.end())
		m_openLinks[linkNo] = std::make_pair(words, std::vector<std::string>());
	else
	{
		assert(it->second.first.size() == 0 && it->second.second.size() > 0);
		s = it->second.second;
		m_openLinks.erase(it);
	}
	
	return s;
}

const std::vector<std::string> WordDependencyState::ProcessReferent(size_t linkNo, const std::vector<std::string> &words)
{
	std::vector<std::string> s;
	
	LinkMap::iterator it = m_openLinks.find(linkNo);
	if(it == m_openLinks.end())
		m_openLinks[linkNo] = std::make_pair(std::vector<std::string>(), words);
	else
	{
		assert(it->second.first.size() > 0 && it->second.second.size() == 0);
		s = it->second.first;
		m_openLinks.erase(it);
	}
	
	return s;
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
