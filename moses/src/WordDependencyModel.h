// vi:ts=2:
#ifndef moses_WordDependencyModel_h
#define moses_WordDependencyModel_h

#include <string>
#include <vector>
#include "FeatureFunction.h"

namespace Moses
{

class Hypothesis;
class InputType;

class WordDependencyModel : public StatefulFeatureFunction {
public:
	WordDependencyModel(FactorType linkFactor, 
										const std::vector<FactorType>& e_factors, 
										const LanguageModel *lm)
				: m_linkFactor(linkFactor), m_eFactors(e_factors), m_lm(lm) {}

	virtual ~WordDependencyModel() {
		delete m_lm;
	}

	virtual size_t GetNumScoreComponents() const {
		return 1;
	}

	virtual const FFState* Evaluate(const Hypothesis& cur_hypo,
														const FFState* prev_state,
														ScoreComponentCollection* accumulator) const;

	virtual const FFState* EmptyHypothesisState(const InputType &input) const;
    
	virtual std::string GetScoreProducerDescription() const {
		return "WordDependencyModel";
	}

	std::string GetScoreProducerWeightShortName() const {
		return "wd";
	};
    
private:
	const FactorType m_linkFactor;
	const std::vector<FactorType> m_eFactors;
	const LanguageModel *m_lm;
	
	const Scores LookupScores(const std::string &antecedent, const std::string &referent) const;
	
	friend class WordDependencyState;
};

}

#endif
