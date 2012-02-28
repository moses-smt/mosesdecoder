#ifndef moses_WordTranslationFeature_h
#define moses_WordTranslationFeature_h

#include <string>
#include <map>

#include "FeatureFunction.h"
#include "FactorCollection.h"

#include "Sentence.h"
#include "FFState.h"

#ifdef WITH_THREADS
#include <boost/thread/tss.hpp>
#endif

namespace Moses
{

/** Sets the features for word translation
 */
//class WordTranslationFeature : public StatelessFeatureFunction {
class WordTranslationFeature : public StatefulFeatureFunction {

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
  std::set<std::string> m_vocabTarget;
  FactorType m_factorTypeSource;
  FactorType m_factorTypeTarget;
  bool m_unrestricted;
  bool m_sourceContext;
  bool m_biphrase;
  bool m_bitrigger;

public:
	WordTranslationFeature(FactorType factorTypeSource = 0, FactorType factorTypeTarget = 0, size_t context = 0):
//     StatelessFeatureFunction("wt", ScoreProducer::unlimited),
		 StatefulFeatureFunction("wt", ScoreProducer::unlimited),
     m_factorTypeSource(factorTypeSource),
     m_factorTypeTarget(factorTypeTarget),
     m_unrestricted(true)
  {
		m_sourceContext = false;
		m_biphrase = false;
		switch(context) {
			case 1:
				m_sourceContext = true;
				std::cerr << "Using source context for word translation feature.." << std::endl;
				break;
			case 2:
				m_biphrase = true;
				std::cerr << "Using biphrases for word translation feature.." << std::endl;
				break;
			case 3:
				m_bitrigger = true;
				std::cerr << "Using bitriggers for word translation feature.." << std::endl;
				break;
		}
  }
      
	bool Load(const std::string &filePathSource, const std::string &filePathTarget);

	void InitializeForInput( Sentence const& in );

//  void Evaluate(const TargetPhrase& cur_phrase, ScoreComponentCollection* accumulator) const;

  const FFState* EmptyHypothesisState(const InputType &) const {
  	return new DummyState();
  }

  FFState* Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state,
                          ScoreComponentCollection* accumulator) const;

  FFState* EvaluateChart( const ChartHypothesis& /* cur_hypo */, int /* featureID */,
                                  ScoreComponentCollection* ) const {
  	/* Not implemented */
    assert(0);
    return NULL;
  }

  // basic properties
	std::string GetScoreProducerWeightShortName(unsigned) const { return "wt"; }
	size_t GetNumInputScores() const { return 0; }

	void AddFeature(ScoreComponentCollection* accumulator, std::string sourceTrigger,
			std::string sourceWord, std::string targetTrigger, std::string targetWord) const;
};

}

#endif // moses_WordTranslationFeature_h
