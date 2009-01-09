// $Id$

#include <cassert>
#include "StaticData.h"
#include "DummyScoreProducers.h"
#include "InputFileStream.h"
#include "LexicalDistortionCost.h"
#include "WordsRange.h"

LexicalDistortionCost::LexicalDistortionCost(const std::string &filePath,
		Direction direction,
		Condition condition,
		std::vector< FactorType >& f_factors,
		std::vector< FactorType >& e_factors,
		size_t numParametersPerDirection) :
        m_modelFileName(filePath),
	m_direction(direction), m_condition(condition),
	m_srcfactors(f_factors), m_tgtfactors(e_factors),
	m_defaultDistortion(4,12),
	m_numParametersPerDirection(numParametersPerDirection)
{
	LoadTable(filePath);
}

LexicalDistortionCost::~LexicalDistortionCost()
{
	for(_DistortionTableType::iterator i = m_distortionTableForward.begin(); i != m_distortionTableForward.end(); ++i)
		delete i->second;
	for(_DistortionTableType::iterator i = m_distortionTableBackward.begin(); i != m_distortionTableBackward.end(); ++i)
		delete i->second;
}

bool LexicalDistortionCost::LoadTable(std::string fileName)
{
	if(!FileExists(fileName) && FileExists(fileName+".gz"))
		fileName += ".gz";
	InputFileStream file(fileName);
	std::string line("");
	std::cerr << "Loading distortion table into memory...";

	assert(m_condition == Word || m_condition == SourcePhrase || m_condition == PhrasePair);
	assert(m_direction == Forward || m_direction == Backward || m_direction == Bidirectional);
	size_t nump = GetNumParameterSets() * GetNumParametersPerDirection();
	while(!getline(file, line).eof()) {
		std::vector<std::string> tokens = TokenizeMultiCharSeparator(line, "|||");
		std::string key;
		std::vector<float> p;
		if(m_condition == Word || m_condition == PhrasePair) {
			key = tokens.at(0) + " ||| " + tokens.at(1);
			//last token are the probs
			p = Scan<float>(Tokenize(tokens.at(2)));
		} else if(m_condition == SourcePhrase) {
			key = tokens.at(0);
			p = Scan<float>(Tokenize(tokens.at(1)));
		}

		if(p.size() != nump) {
			TRACE_ERR( "found inconsistent number of parameters... found " << p.size()
				<< " expected " << nump << std::endl);
			return false;
		}

		if(m_direction == Forward)
			m_distortionTableForward[key] = new std::vector<float>(p);
		else if(m_direction == Backward)
			m_distortionTableBackward[key] = new std::vector<float>(p);
		else if(m_direction == Bidirectional) {
			m_distortionTableForward[key] = new std::vector<float>(p.begin(),
				p.begin() + GetNumParametersPerDirection());
			m_distortionTableBackward[key] = new std::vector<float>(p.begin() + GetNumParametersPerDirection(),
				p.end());
		}
	}
	std::cerr << "done.\n";
	return true;
}

const std::vector<float> *LexicalDistortionCost::GetDistortionParameters(std::string key, Direction dir) const
{
	assert(dir == Forward || dir == Backward);

	const _DistortionTableType &tab = dir == Forward ? m_distortionTableForward : m_distortionTableBackward;

	_DistortionTableType::const_iterator i = tab.find(key);
	if(i != tab.end()) {
		// std::cerr << "found *" << s << "*: " << i->second.first << "/" << i->second.second << std::endl;
		return i->second;
	} else {
		// std::cerr << "not found: *" << s << "*" << std::endl;
		// static const float default_distortion[4] = {8.23, 8, 8.23, 8};
		// static const float default_distortion[4] = {12, 12, 12, 12};
		return &m_defaultDistortion;
	}
}

std::vector<float> LexicalDistortionCost::CalcScore(Hypothesis *h) const
{
	std::vector<float> scores;
	std::string key;

	if(m_direction == Forward || m_direction == Bidirectional) {
		if(m_condition == Word)
			key = h->GetSourcePhrase()->GetWord(0).ToString();
		else if(m_condition == SourcePhrase)
			key = h->GetSourcePhraseStringRep(m_srcfactors);
		else if(m_condition == PhrasePair)
			key = h->GetSourcePhraseStringRep(m_srcfactors) + " ||| " +
				h->GetTargetPhraseStringRep(m_tgtfactors);

		const std::vector<float> *params_cur = GetDistortionParameters(key, Forward);
		scores.push_back(CalculateDistortionScore(
			h->GetPrevHypo()->GetCurrSourceWordsRange(),
			h->GetCurrSourceWordsRange(), params_cur));
	}

	if(m_direction == Backward || m_direction == Bidirectional) {
		if(h->GetPrevHypo()->GetPrevHypo() == NULL)
			scores.push_back(.0f);
		else {
			assert(m_condition == Word || m_condition == SourcePhrase || m_condition == PhrasePair);
			if(m_condition == Word) {
				const Phrase* prevSrcPhrase = h->GetPrevHypo()->GetSourcePhrase();
				key = prevSrcPhrase->GetWord(prevSrcPhrase->GetSize() - 1).ToString();
			} else if(m_condition == SourcePhrase)
				key = h->GetPrevHypo()->GetSourcePhraseStringRep(m_srcfactors);
			else if(m_condition == PhrasePair)
				key = h->GetPrevHypo()->GetSourcePhraseStringRep(m_srcfactors) + " ||| " +
					h->GetPrevHypo()->GetTargetPhraseStringRep(m_tgtfactors);

			const std::vector<float> *params_prev = GetDistortionParameters(key, Backward);
			scores.push_back(CalculateDistortionScore(
					h->GetPrevHypo()->GetCurrSourceWordsRange(),
					h->GetCurrSourceWordsRange(), params_prev));
		}
	}

	return scores;
}

float LDCBetaBinomial::CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr,
	const std::vector<float> *params) const
{
        // float p = px + 2.23;
        float p = params->at(0) + 2;
        float q = params->at(1) + 2;

        int x = StaticData::Instance().GetInput()->ComputeDistortionDistance(prev, curr);
        if(x < -m_distortionRange) x = -m_distortionRange;
        if(x >  m_distortionRange) x =  m_distortionRange;
        x += m_distortionRange;

        return beta_binomial(p, q, x);
}

float LDCBetaBinomial::beta_binomial(float p, float q, int x) const
{
        const long double n = (long double) 2 * m_distortionRange;

        long double n1 = lgammal(n + 1);
        long double n2 = lgammal(q + n - x);
        long double n3 = lgammal(x + p);
        long double n4 = lgammal(p + q);
        long double d1 = lgammal(x + 1);
        long double d2 = lgammal(n - x + 1);
        long double d3 = lgammal(p + q + n);
        long double d4 = lgammal(p);
        long double d5 = lgammal(q);

        // std::cerr << n1 << " + " << n2 << " + " << n3 << " + " << n4 << std::endl;
        // std::cerr << d1 << " + " << d2 << " + " << d3 << " + " << d4 << " + " << d5 << std::endl;

        long double part1 = n1 + n4 - d3 - d4 - d5;
        // std::cerr << part1 << std::endl;
        long double part2 = n2 + n3 - d1 - d2;
        // std::cerr << part2 << std::endl;

        long double score = part1 + part2;
        // std::cerr << "x: " << x << "   score: " << score << std::endl;

        if(!finite(score)) {
                std::cerr << p << " ; " << q << std::endl;
                std::cerr << n1 << " + " << n2 << " + " << n3 << " + " << n4 << std::endl;
                std::cerr << d1 << " + " << d2 << " + " << d3 << " + " << d4 << " + " << d5 << std::endl;
                std::cerr << part1 << std::endl;
                std::cerr << part2 << std::endl;
                std::cerr << "x: " << x << "   score: " << score << std::endl;
                assert(false);
        }

        return FloorScore((float) score);
}
