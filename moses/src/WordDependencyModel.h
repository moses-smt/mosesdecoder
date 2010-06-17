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
										const std::vector<FactorType>& e_factors_ant, 
										const std::vector<FactorType>& e_factors_ref, 
										FactorType alignmentFactor, 
										const LanguageModel *lm)
				: m_linkFactor(linkFactor), m_alignmentFactor(alignmentFactor),
					m_eFactorsAntecedent(e_factors_ant), m_eFactorsReferent(e_factors_ref), m_lm(lm) {}

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
	const FactorType m_alignmentFactor;
	const std::vector<FactorType> m_eFactorsAntecedent;
	const std::vector<FactorType> m_eFactorsReferent;
	const LanguageModel *m_lm;
	
	const Scores LookupScores(const std::vector<std::string> &antecedent, const std::vector<std::string> &referent) const;
	std::vector<std::string> GetAlignedTargetWords(const Hypothesis &hypo, size_t pos, bool referent) const;
	
	friend class WordDependencyState;
};

}

#endif
