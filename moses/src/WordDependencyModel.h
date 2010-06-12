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
	WordDependencyModel( std::vector<FactorType>& f_factors, 
										std::vector<FactorType>& e_factors, 
										FactorType link_factor,
										const std::string &filePath,
										const std::vector<float> &weights);

	virtual ~WordDependencyModel();

	virtual size_t GetNumScoreComponents() const {
		return 1;
	}

	virtual FFState* Evaluate(const Hypothesis& cur_hypo,
														const FFState* prev_state,
														ScoreComponentCollection* accumulator) const;

	virtual const FFState* EmptyHypothesisState(const InputType &input) const;
    
	virtual std::string GetScoreProducerDescription() const {
		return "CoreferenceModel";
	}

	std::string GetScoreProducerWeightShortName() const {
		return "cr";
	};
    
private:
	std::vector<FactorType> m_factorsE, m_factorsF;
};

}

#endif
