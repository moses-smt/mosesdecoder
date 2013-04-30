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

#include "FFState.h"

#ifdef WITH_THREADS
#include <boost/thread/tss.hpp>
#endif

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
	typedef std::map< char, short > CharHash;
	typedef std::map< std::string, short > StringHash;

  struct ThreadLocalStorage
  {
    const Sentence *input;
  };

private:
#ifdef WITH_THREADS
  boost::thread_specific_ptr<ThreadLocalStorage> m_local;
#else
  std::auto_ptr<ThreadLocalStorage> m_local;
#endif

  CharHash m_punctuationHash;

  std::vector< FactorType > m_inputFactors;
  std::vector< FactorType > m_outputFactors;
  bool m_unrestricted;

  bool m_sourceContext;
  bool m_biphrase;
  bool m_bitrigger;

  bool m_biasFeature;
  float m_sparseProducerWeight;
  bool m_ignorePunctuation;

  std::set<std::string> m_vocabSource;
  std::set<std::string> m_vocabTarget;

public:
  GlobalLexicalModelUnlimited(const std::vector< FactorType >& inFactors, const std::vector< FactorType >& outFactors,
  		bool biasFeature, bool ignorePunctuation, size_t context):
    StatelessFeatureFunction("glm",ScoreProducer::unlimited),
  	m_inputFactors(inFactors),
    m_outputFactors(outFactors),
    m_unrestricted(true),
    m_sourceContext(false),
    m_biphrase(false),
    m_bitrigger(false),
    m_biasFeature(biasFeature),
    m_sparseProducerWeight(1),
    m_ignorePunctuation(ignorePunctuation)
  {
  	std::cerr << "Creating global lexical model unlimited.. ";


		switch(context) {
			case 1:
				m_sourceContext = true;
				std::cerr << "using source context.. ";
				break;
			case 2:
				m_biphrase = true;
				std::cerr << "using biphrases.. ";
				break;
			case 3:
				std::cerr << "using bitriggers.. ";
				m_bitrigger = true;
				break;
		}

  	// compile a list of punctuation characters
  	if (m_ignorePunctuation) {
  		std::cerr << "ignoring punctuation.. ";
  		char punctuation[] = "\"'!?¿·()#_,.:;•&@‑/\\0123456789~=";
  		for (size_t i=0; i < sizeof(punctuation)-1; ++i)
  			m_punctuationHash[punctuation[i]] = 1;
  	}
  	std::cerr << "done." << std::endl;
  }

  std::string GetScoreProducerWeightShortName(unsigned) const {
    return "glm";
  };

  bool Load(const std::string &filePathSource, const std::string &filePathTarget);

  void InitializeForInput( Sentence const& in );

  const FFState* EmptyHypothesisState(const InputType &) const {
  	return new DummyState();
  }

  //TODO: This implements the old interface, but cannot be updated because
  //it appears to be stateful
  void Evaluate(const Hypothesis& cur_hypo,
  							ScoreComponentCollection* accumulator) const;

  void EvaluateChart(const ChartHypothesis& /* cur_hypo */,
  									 int /* featureID */,
  									 ScoreComponentCollection* ) const {
  	/* Not implemented */
    assert(0);
  }


  void SetSparseProducerWeight(float weight) { m_sparseProducerWeight = weight; }
  float GetSparseProducerWeight() const { return m_sparseProducerWeight; }

	void AddFeature(ScoreComponentCollection* accumulator, StringHash alreadyScored,
			std::string sourceTrigger, std::string sourceWord, std::string targetTrigger,
			std::string targetWord) const;
};

}
#endif /* GLOBALLEXICALMODELUNLIMITED_H_ */
