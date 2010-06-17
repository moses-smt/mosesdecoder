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
#include "StaticData.h"
#include "TypeDef.h"
#include "Util.h"

namespace Moses
{

class WordDependencyState : public FFState
{
private:
	typedef std::map<size_t, std::vector<std::string> > AntecedentMap;
	typedef std::multimap<size_t, std::vector<std::string> > ReferentMap;
	
	AntecedentMap m_antecedents;
	ReferentMap m_openReferents;
	const WordDependencyModel *m_model;

public:
	WordDependencyState(const WordDependencyModel	*model) : m_model(model) {}

	WordDependencyState *Clone() const;
	const std::vector<std::vector<std::string> > ProcessAntecedent(size_t linkNo, const std::vector<std::string> &word);
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
			const std::vector<std::string> antwords = GetAlignedTargetWords(cur_hypo, i, false);
			const std::vector<std::vector<std::string> > refwords = new_state->ProcessAntecedent(linkNo, antwords);
			for(size_t j = 0; j < refwords.size(); j++)
				accumulator->PlusEquals(this, LookupScores(antwords, refwords[j]));
		}

		if(ref != "*")
		{
			if(ref[0] != '>')
			{
				size_t linkNo = Scan<size_t>(ref);
				const std::vector<std::string> refwords = GetAlignedTargetWords(cur_hypo, i, true);
				const std::vector<std::string> antwords = new_state->ProcessReferent(linkNo, refwords);
				if(antwords.size() > 0)
					accumulator->PlusEquals(this, LookupScores(antwords, refwords));
			}
			else
			{
				const std::vector<std::string> antwords = Tokenize(ref.substr(1), "~");
				const std::vector<std::string> refwords = GetAlignedTargetWords(cur_hypo, i, true);
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
	Scores s(1, LOWEST_SCORE);

	if(antecedent.size() == 0 || referent.size() == 0)
	{
		VERBOSE(2, "Unaligned trigger word for word dependency model." << std::endl);
		return s;
	}

	Word antw, refw;
	std::vector<const Word *> bigram(2);
	const std::vector<FactorType> singleFactor(1, 0);
	
	bigram[0] = &antw;
	bigram[1] = &refw;

	for(size_t i = 0; i < antecedent.size(); i++) {
		antw.CreateFromString(Output, singleFactor, antecedent[i], false);
		for(size_t j = 0; j < referent.size(); j++) {
			refw.CreateFromString(Output, singleFactor, referent[j], false);
			float score = m_lm->GetValue(bigram);
			std::cerr << "Word dependency score for antecedent " << antecedent[i] << " and referent " << referent[j] << ": " << score << std::endl;
			if(score > s[0])
				s[0] = score;
		}
	}
	std::cerr << "Maximum score: " << s[0] << std::endl;
	
	return s;
}

std::vector<std::string> WordDependencyModel::GetAlignedTargetWords(const Hypothesis &hypo, size_t pos, bool referent) const
{
	const std::vector<FactorType> &eFactors = referent ? m_eFactorsReferent : m_eFactorsAntecedent;
	std::vector<std::string> ret;
	const TargetPhrase tp = hypo.GetTargetPhrase();
	for(unsigned i = 0; i < tp.GetSize(); i++)
	{
		std::string alig = tp.GetFactor(i, m_alignmentFactor)->GetString();
		if(alig == "-")
			continue;
		std::vector<unsigned> apos = Tokenize<unsigned>(alig, ",");
		for(unsigned j = 0; j < apos.size(); j++)
			if(apos[j] == pos)
				ret.push_back(tp.GetWord(i).GetString(eFactors, false));
	}
	return ret;
}

WordDependencyState *WordDependencyState::Clone() const
{
	return new WordDependencyState(*this);
}

const std::vector<std::vector<std::string> > WordDependencyState::ProcessAntecedent(size_t linkNo, const std::vector<std::string> &words)
{
	std::vector<std::vector<std::string> > s;
	
	m_antecedents.insert(std::make_pair(linkNo, words));

	std::pair<ReferentMap::iterator,ReferentMap::iterator> range = m_openReferents.equal_range(linkNo);
	for(ReferentMap::iterator it = range.first; it != range.second; ++it)
		s.push_back(it->second);

	m_openReferents.erase(range.first, range.second);

	return s;
}

const std::vector<std::string> WordDependencyState::ProcessReferent(size_t linkNo, const std::vector<std::string> &words)
{
	std::vector<std::string> s;
	
	AntecedentMap::iterator it = m_antecedents.find(linkNo);
	if(it == m_antecedents.end())
		m_openReferents.insert(std::make_pair(linkNo, words));
	else
		s = it->second;
	
	return s;
}

int WordDependencyState::Compare(const FFState &other) const
{
	const WordDependencyState &o = dynamic_cast<const WordDependencyState &>(other);
	
	if(&o == this)
		return 0;

	assert(m_model == o.m_model);
	
	if(m_antecedents.size() < o.m_antecedents.size())
		return -1;
	else if(m_antecedents.size() > o.m_antecedents.size())
		return 1;
	
	if(m_openReferents.size() < o.m_openReferents.size())
		return -1;
	else if(m_openReferents.size() > o.m_openReferents.size())
		return 1;
	
	AntecedentMap::const_iterator i, j;
	for(i = m_antecedents.begin(), j = o.m_antecedents.begin(); i != m_antecedents.end(); ++i, ++j)
		if(*i < *j)
			return -1;
		else if(*i > *j)
			return 1;
	
	ReferentMap::const_iterator k, l;
	for(k = m_openReferents.begin(), l = o.m_openReferents.begin(); k != m_openReferents.end(); ++k, ++l)
		if(*k < *l)
			return -1;
		else if(*k > *l)
			return 1;
	
	return 0;
}

}
