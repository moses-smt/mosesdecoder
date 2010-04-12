#ifndef moses_GlobalLexicalModel_h
#define moses_GlobalLexicalModel_h

#include <string>
#include <vector>
#include "Factor.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "Util.h"
#include "WordsRange.h"
#include "ScoreProducer.h"
#include "FeatureFunction.h"
#include "FactorTypeSet.h"
#include "Sentence.h"

namespace Moses
{

class Factor;
class Phrase;
class Hypothesis;
class InputType;

/** Discriminatively trained global lexicon model
 * This is a implementation of Mauser et al., 2009's model that predicts
 * each output word from _all_ the input words. The intuition behind this
 * feature is that it uses context words for disambiguation
 */

class GlobalLexicalModel : public StatelessFeatureFunction {
	typedef std::map< const Word*, std::map< const Word*, float, WordComparer >, WordComparer > DoubleHash;
	typedef std::map< const Word*, float, WordComparer > SingleHash;
private:
	DoubleHash m_hash;
	std::map< const TargetPhrase*, float > *m_cache;
	const Sentence *m_input;
	Word *m_bias;
	
	FactorMask m_inputFactors;
	FactorMask m_outputFactors;

	void LoadData(const std::string &filePath,
	              const std::vector< FactorType >& inFactors,
	              const std::vector< FactorType >& outFactors);
	
	float ScorePhrase( const TargetPhrase& targetPhrase ) const;
	float GetFromCacheOrScorePhrase( const TargetPhrase& targetPhrase ) const;

public:
	GlobalLexicalModel(const std::string &filePath,
	                   const float weight,
	                   const std::vector< FactorType >& inFactors,
	                   const std::vector< FactorType >& outFactors);
	virtual ~GlobalLexicalModel();

	virtual size_t GetNumScoreComponents() const {
		return 1;
	};

	virtual std::string GetScoreProducerDescription() const {
		return "GlobalLexicalModel";
	};

	virtual std::string GetScoreProducerWeightShortName() const {
		return "lex";
	};

	void InitializeForInput( Sentence const& in );
	
	void Evaluate(const TargetPhrase&, ScoreComponentCollection* ) const;
};

}
#endif
