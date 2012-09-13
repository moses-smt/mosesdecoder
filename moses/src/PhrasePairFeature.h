#ifndef moses_PhrasePairFeature_h
#define moses_PhrasePairFeature_h

#include "Factor.h"
#include "FeatureFunction.h"
#include "Sentence.h"

#ifdef WITH_THREADS
#include <boost/thread/tss.hpp>
#endif

namespace Moses {

/**
  * Phrase pair feature: complete source/target phrase pair
  **/
class PhrasePairFeature: public StatelessFeatureFunction {
	
	typedef std::map< char, short > CharHash;
	typedef std::vector< std::set<std::string> > DocumentVector;
	
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

	  std::set<std::string> m_vocabSource;
	  //std::set<std::string> m_vocabTarget;
	  FactorType m_sourceFactorId;
	  FactorType m_targetFactorId;
	  bool m_unrestricted;
	  bool m_simple;
	  bool m_sourceContext;
	  float m_sparseProducerWeight;
	  bool m_ignorePunctuation;
	  CharHash m_punctuationHash;
	
  public:
	  PhrasePairFeature (FactorType sourceFactorId, FactorType targetFactorId, 
			     bool simple, bool sourceContext, bool ignorePunctuation) :
				  StatelessFeatureFunction("pp", ScoreProducer::unlimited),
	    m_sourceFactorId(sourceFactorId),
	    m_targetFactorId(targetFactorId),
	    m_unrestricted(true),
	    m_simple(simple),
	    m_sourceContext(sourceContext),		    
	    m_sparseProducerWeight(1),
	    m_ignorePunctuation(ignorePunctuation) {
		  std::cerr << "Creating phrase pair feature.. " << std::endl;
		  if (m_simple == 1) std::cerr << "using simple phrase pairs.. ";
		  if (m_sourceContext == 1) std::cerr << "using source context.. ";
		  
		  // compile a list of punctuation characters 
		  if (m_ignorePunctuation) {
			  std::cerr << "ignoring punctuation for triggers.. ";
			  char punctuation[] = "\"'!?¿·()#_,.:;•&@‑/\\0123456789~=";
			  for (size_t i=0; i < sizeof(punctuation)-1; ++i)
				  m_punctuationHash[punctuation[i]] = 1;
		  }		  
	  }

    void Evaluate(const TranslationOption& translationOption,
                const InputType& inputType,
                const WordsBitmap& coverageVector,
                ScoreComponentCollection* accumulator) const;

    void EvaluateChart(
      const ChartHypothesis&,
      int /* featureID */,
      ScoreComponentCollection*) const {
  		CHECK(0); // feature function not valid in chart decoder
  	}

    bool ComputeValueInTranslationOption() const;
    
    std::string GetScoreProducerWeightShortName(unsigned) const;
    size_t GetNumInputScores() const;

    bool Load(const std::string &filePathSource/*, const std::string &filePathTarget*/);

    void InitializeForInput( Sentence const& in );
    
    void SetSparseProducerWeight(float weight) { m_sparseProducerWeight = weight; }
    float GetSparseProducerWeight() const { return m_sparseProducerWeight; }
};

}


#endif
