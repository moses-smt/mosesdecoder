#ifndef moses_PhrasePairFeature_h
#define moses_PhrasePairFeature_h

#include <stdexcept>

#include "Factor.h"
#include "FeatureFunction.h"
#include "Sentence.h"

namespace Moses {

/**
  * Phrase pair feature: complete source/target phrase pair
  **/
class PhrasePairFeature: public StatelessFeatureFunction {
    
    typedef std::map< char, short > CharHash;
    typedef std::vector< std::set<std::string> > DocumentVector;
       
    std::set<std::string> m_vocabSource;
    //std::set<std::string> m_vocabTarget;
    DocumentVector m_vocabDomain;
    FactorType m_sourceFactorId;
    FactorType m_targetFactorId;
    bool m_unrestricted;
    bool m_simple;
    bool m_sourceContext;
    bool m_domainTrigger;
    float m_sparseProducerWeight;
    bool m_ignorePunctuation;
    CharHash m_punctuationHash;
    
  public:
    PhrasePairFeature (FactorType sourceFactorId, FactorType targetFactorId, 
		       bool simple, bool sourceContext, bool ignorePunctuation, bool domainTrigger) :
    StatelessFeatureFunction("pp", ScoreProducer::unlimited),
      m_sourceFactorId(sourceFactorId),
      m_targetFactorId(targetFactorId),
      m_unrestricted(true),
      m_simple(simple),
      m_sourceContext(sourceContext),	    
      m_domainTrigger(domainTrigger),		    
      m_sparseProducerWeight(1),
      m_ignorePunctuation(ignorePunctuation) {
	std::cerr << "Creating phrase pair feature.. " << std::endl;
	if (m_simple == 1) std::cerr << "using simple phrase pairs.. ";
	if (m_sourceContext == 1) std::cerr << "using source context.. ";
	if (m_domainTrigger == 1) std::cerr << "using domain triggers.. ";
	
	// compile a list of punctuation characters 
	if (m_ignorePunctuation) {
	  std::cerr << "ignoring punctuation for triggers.. ";
	  char punctuation[] = "\"'!?¿·()#_,.:;•&@‑/\\0123456789~=";
	  for (size_t i=0; i < sizeof(punctuation)-1; ++i)
	    m_punctuationHash[punctuation[i]] = 1;
	}		  
      }
    
    void Evaluate(const PhraseBasedFeatureContext& context,
                  ScoreComponentCollection* accumulator) const;
    
    void EvaluateChart(const ChartBasedFeatureContext& context,
                       ScoreComponentCollection*) const {
      throw std::logic_error("PhrasePairFeature not valid in chart decoder");
    }
    
    bool ComputeValueInTranslationOption() const;
    
    std::string GetScoreProducerWeightShortName(unsigned) const;
    size_t GetNumInputScores() const;

    bool Load(const std::string &filePathSource/*, const std::string &filePathTarget*/);

    void SetSparseProducerWeight(float weight) { m_sparseProducerWeight = weight; }
    float GetSparseProducerWeight() const { return m_sparseProducerWeight; }
};

}


#endif
