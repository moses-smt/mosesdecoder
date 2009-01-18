// $Id$

#include <cassert>
#include "StaticData.h"
#include "InputFileStream.h"
#include "LexicalDistortionCost.h"
#include "LexicalReorderingTable.h"
#include "WordsRange.h"
#include "TranslationOption.h"


LexicalDistortionCost *LexicalDistortionCost::CreateModel(const std::string &modelType,
                                const std::string &filePath,
                                Direction direction,
                                std::vector< FactorType >& f_factors,
                                std::vector< FactorType >& e_factors)
{
	if(modelType == "beta-binomial-phrase")
		return new LDCBetaBinomial(filePath, direction, PhrasePair, f_factors, e_factors);
	else if(modelType == "beta-geometric-phrase")
		return new LDCBetaGeometric(filePath, direction, PhrasePair, f_factors, e_factors);
	else {
		UserMessage::Add("Lexical distortion model type not implemented: " + modelType);
		return NULL;
	}
}

LexicalDistortionCost::LexicalDistortionCost(const std::string &filePath,
		Direction direction,
		Condition condition,
		std::vector< FactorType >& f_factors,
		std::vector< FactorType >& e_factors,
		size_t numParametersPerDirection) :
        m_modelFileName(filePath),
	m_direction(direction), m_condition(condition),
	m_srcfactors(f_factors), m_tgtfactors(e_factors),
	m_numParametersPerDirection(numParametersPerDirection)
{
	m_distortionTable = LexicalReorderingTable::LoadAvailable(filePath, f_factors, e_factors, std::vector<FactorType>(), false);
}

LexicalDistortionCost::~LexicalDistortionCost()
{
	delete m_distortionTable;
}

const std::vector<float> LexicalDistortionCost::GetDistortionParameters(const Phrase &src, const Phrase &tgt) const
{
	std::vector<float> params = m_distortionTable->GetScore(src, tgt, Phrase(Output));
	if(params.size() == 0) return GetDefaultDistortion();

	assert(params.size() == GetNumParameterSets() * GetNumParametersPerDirection());

	return params;
}

std::vector<float> LexicalDistortionCost::CalcScore(Hypothesis *h) const
{
	std::vector<float> scores;

	if(m_direction == Forward || m_direction == Bidirectional) {
		std::vector<float> params_cur = h->GetTranslationOption().GetDistortionParamsForModel(this);
		assert(params_cur.size() == GetNumParameterSets() * GetNumParametersPerDirection());
		std::vector<float> params_pre(params_cur.begin(), params_cur.begin() + GetNumParametersPerDirection());
		scores.push_back(CalculateDistortionScore(
			h->GetPrevHypo()->GetCurrSourceWordsRange(),
			h->GetCurrSourceWordsRange(), &params_pre));
	}

	if(m_direction == Backward || m_direction == Bidirectional) {
		if(h->GetPrevHypo()->GetPrevHypo() == NULL)
			scores.push_back(.0f);
		else {
			std::vector<float> params_prev = h->GetPrevHypo()->GetTranslationOption().GetDistortionParamsForModel(this);
			assert(params_prev.size() == GetNumParameterSets() * GetNumParametersPerDirection());
			std::vector<float> params_post(params_prev.begin() + GetNumParametersPerDirection(), params_prev.end());
			scores.push_back(CalculateDistortionScore(
					h->GetPrevHypo()->GetCurrSourceWordsRange(),
					h->GetCurrSourceWordsRange(), &params_post));
		}
	}

	return scores;
}
	
std::vector<float> LexicalDistortionCost::GetProb(const Phrase &src, const Phrase &tgt) const {
	assert(m_condition == PhrasePair); // everything else isn't implemented right now
	return GetDistortionParameters(src, tgt);
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

std::vector<float> LDCBetaBinomial::GetDefaultDistortion() const
{
	return std::vector<float>(4, 12);
}

float LDCBetaGeometric::CalculateDistortionScore(const WordsRange &prev, const WordsRange &curr,
	const std::vector<float> *params) const
{
	float p_dir = params->at(0);
	float q_dir = params->at(1);
	float p_pos = params->at(2);
	float q_pos = params->at(3);
	float p_neg = params->at(4);
	float q_neg = params->at(5);

	int distance = StaticData::Instance().GetInput()->ComputeDistortionDistance(prev, curr);

	float score = .0f;

	if(distance >= 0) {
		score += log(p_dir / (p_dir + q_dir));

		int x = distance;
		score += beta_geometric(p_pos, q_pos, x);
	} else {
		score += log(1.0f - p_dir / (p_dir + q_dir));

		int x = -distance - 1;
		score += beta_geometric(p_neg, q_neg, x);
	}

	assert(finite(score));
	return FloorScore(score);
}

float LDCBetaGeometric::beta_geometric(float p, float q, int x) const
{
	assert(x >= 0);

	float result = log(p);
	for(int i = 0; i <= x - 1; i++)
		result += log(q + i);
	for(int i = 0; i <= x; i++)
		result -= log(p + q + i);
	return result;
}

std::vector<float> LDCBetaGeometric::GetDefaultDistortion() const
{
	return std::vector<float>(12, 2);
}
