#ifndef GLOBALLEXICALMODELUNLIMITED_H_
#define GLOBALLEXICALMODELUNLIMITED_H_

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

class GlobalLexicalModelUnlimited : public StatelessFeatureFunction
{
//	typedef std::map< char, short > CharHash;
	typedef std::map< std::string, short > StringHash;
private:
  const Sentence *m_input;

//  CharHash m_punctuationHash;

  std::vector< FactorType > m_inputFactors;
  std::vector< FactorType > m_outputFactors;

  float m_sparseProducerWeight;

  void LoadData(const std::vector< FactorType >& inFactors,
                const std::vector< FactorType >& outFactors);

public:
  GlobalLexicalModelUnlimited(const std::vector< FactorType >& inFactors,
	                            const std::vector< FactorType >& outFactors);

  virtual ~GlobalLexicalModelUnlimited();

  virtual std::string GetScoreProducerWeightShortName(unsigned) const {
    return "glm";
  };

  void InitializeForInput( Sentence const& in );

  void Evaluate(const TargetPhrase&, ScoreComponentCollection* ) const;

  void SetSparseProducerWeight(float weight) { m_sparseProducerWeight = weight; }
  float GetSparseProducerWeight() const { return m_sparseProducerWeight; }
};

}
#endif /* GLOBALLEXICALMODELUNLIMITED_H_ */
