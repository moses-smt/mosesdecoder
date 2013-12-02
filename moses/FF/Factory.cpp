#include "moses/FF/Factory.h"
#include "moses/StaticData.h"

#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryOnDisk.h"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"
#include "moses/TranslationModel/PhraseDictionaryMultiModel.h"
#include "moses/TranslationModel/PhraseDictionaryMultiModelCounts.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryALSuffixArray.h"
#include "moses/TranslationModel/PhraseDictionaryDynSuffixArray.h"
#include "moses/TranslationModel/PhraseDictionaryScope3.h"
#include "moses/TranslationModel/TransliterationPhraseDictionary.h"

#include "moses/FF/LexicalReordering/LexicalReordering.h"

#include "moses/FF/BleuScoreFeature.h"
#include "moses/FF/TargetWordInsertionFeature.h"
#include "moses/FF/SourceWordDeletionFeature.h"
#include "moses/FF/GlobalLexicalModel.h"
#include "moses/FF/GlobalLexicalModelUnlimited.h"
#include "moses/FF/UnknownWordPenaltyProducer.h"
#include "moses/FF/WordTranslationFeature.h"
#include "moses/FF/TargetBigramFeature.h"
#include "moses/FF/TargetNgramFeature.h"
#include "moses/FF/PhraseBoundaryFeature.h"
#include "moses/FF/PhrasePairFeature.h"
#include "moses/FF/PhraseLengthFeature.h"
#include "moses/FF/DistortionScoreProducer.h"
#include "moses/FF/WordPenaltyProducer.h"
#include "moses/FF/InputFeature.h"
#include "moses/FF/PhrasePenalty.h"
#include "moses/FF/OSM-Feature/OpSequenceModel.h"
#include "moses/FF/ControlRecombination.h"
#include "moses/FF/ExternalFeature.h"
#include "moses/FF/ConstrainedDecoding.h"
#include "moses/FF/CoveredReferenceFeature.h"

#include "moses/FF/SkeletonStatelessFF.h"
#include "moses/FF/SkeletonStatefulFF.h"
#include "moses/LM/SkeletonLM.h"
#include "moses/TranslationModel/SkeletonPT.h"

#ifdef HAVE_CMPH
#include "moses/TranslationModel/CompactPT/PhraseDictionaryCompact.h"
#endif
#ifdef PT_UG
#include "moses/TranslationModel/mmsapt.h"
#endif

#include "moses/LM/Ken.h"
#ifdef LM_IRST
#include "moses/LM/IRST.h"
#endif

#ifdef LM_SRI
#include "moses/LM/SRI.h"
#endif

#ifdef LM_RAND
#include "moses/LM/Rand.h"
#endif

#ifdef HAVE_SYNLM
#include "moses/SyntacticLanguageModel.h"
#endif

#ifdef LM_NEURAL
#include "moses/LM/NeuralLMWrapper.h"
#endif

#ifdef LM_DALM
#include "moses/LM/DALMWrapper.h"
#endif

#include "util/exception.hh"

#include <vector>

namespace Moses
{

class FeatureFactory
{
public:
  virtual ~FeatureFactory() {}

  virtual void Create(const std::string &line) = 0;

protected:
  template <class F> static void DefaultSetup(F *feature);

  FeatureFactory() {}
};

template <class F> void FeatureFactory::DefaultSetup(F *feature)
{
  StaticData &static_data = StaticData::InstanceNonConst();
  const string &featureName = feature->GetScoreProducerDescription();
  std::vector<float> weights = static_data.GetParameter()->GetWeights(featureName);

  if (feature->IsTuneable() || weights.size()) {
    // if it's tuneable, ini file MUST have weights
    // even it it's not tuneable, people can still set the weights in the ini file
    static_data.SetWeights(feature, weights);
  } else {
    std::vector<float> defaultWeights = feature->DefaultWeights();
    static_data.SetWeights(feature, defaultWeights);
  }
}

namespace
{

template <class F> class DefaultFeatureFactory : public FeatureFactory
{
public:
  void Create(const std::string &line) {
    DefaultSetup(new F(line));
  }
};

class KenFactory : public FeatureFactory
{
public:
  void Create(const std::string &line) {
    DefaultSetup(ConstructKenLM(line));
  }
};

} // namespace

FeatureRegistry::FeatureRegistry()
{
// Feature with same name as class
#define MOSES_FNAME(name) Add(#name, new DefaultFeatureFactory< name >());
// Feature with different name than class.
#define MOSES_FNAME2(name, type) Add(name, new DefaultFeatureFactory< type >());
  MOSES_FNAME(GlobalLexicalModel);
  //MOSES_FNAME(GlobalLexicalModelUnlimited); This was commented out in the original
  MOSES_FNAME(SourceWordDeletionFeature);
  MOSES_FNAME(TargetWordInsertionFeature);
  MOSES_FNAME(PhraseBoundaryFeature);
  MOSES_FNAME(PhraseLengthFeature);
  MOSES_FNAME(WordTranslationFeature);
  MOSES_FNAME(TargetBigramFeature);
  MOSES_FNAME(TargetNgramFeature);
  MOSES_FNAME(PhrasePairFeature);
  MOSES_FNAME(LexicalReordering);
  MOSES_FNAME2("Generation", GenerationDictionary);
  MOSES_FNAME(BleuScoreFeature);
  MOSES_FNAME2("Distortion", DistortionScoreProducer);
  MOSES_FNAME2("WordPenalty", WordPenaltyProducer);
  MOSES_FNAME(InputFeature);
  MOSES_FNAME2("PhraseDictionaryBinary", PhraseDictionaryTreeAdaptor);
  MOSES_FNAME(PhraseDictionaryOnDisk);
  MOSES_FNAME(PhraseDictionaryMemory);
  MOSES_FNAME(PhraseDictionaryScope3);
  MOSES_FNAME(PhraseDictionaryMultiModel);
  MOSES_FNAME(PhraseDictionaryMultiModelCounts);
  MOSES_FNAME(PhraseDictionaryALSuffixArray);
  MOSES_FNAME(PhraseDictionaryDynSuffixArray);
  MOSES_FNAME(TransliterationPhraseDictionary);
  MOSES_FNAME(OpSequenceModel);
  MOSES_FNAME(PhrasePenalty);
  MOSES_FNAME2("UnknownWordPenalty", UnknownWordPenaltyProducer);
  MOSES_FNAME(ControlRecombination);
  MOSES_FNAME(ConstrainedDecoding);
  MOSES_FNAME(CoveredReferenceFeature);
  MOSES_FNAME(ExternalFeature);

  MOSES_FNAME(SkeletonStatelessFF);
  MOSES_FNAME(SkeletonStatefulFF);
  MOSES_FNAME(SkeletonLM);
  MOSES_FNAME(SkeletonPT);

#ifdef HAVE_CMPH
  MOSES_FNAME(PhraseDictionaryCompact);
#endif
#ifdef PT_UG
  MOSES_FNAME(Mmsapt);
#endif
#ifdef HAVE_SYNLM
  MOSES_FNAME(SyntacticLanguageModel);
#endif
#ifdef LM_IRST
  MOSES_FNAME2("IRSTLM", LanguageModelIRST);
#endif
#ifdef LM_SRI
  MOSES_FNAME2("SRILM", LanguageModelSRI);
#endif
#ifdef LM_RAND
  MOSES_FNAME2("RANDLM", LanguageModelRandLM);
#endif
#ifdef LM_NEURAL
  MOSES_FNAME2("NeuralLM", NeuralLMWrapper);
#endif
#ifdef LM_DALM
  MOSES_FNAME2("DALM", LanguageModelDALM);
#endif

  Add("KENLM", new KenFactory());
}

FeatureRegistry::~FeatureRegistry()
{
}

void FeatureRegistry::Add(const std::string &name, FeatureFactory *factory)
{
  std::pair<std::string, boost::shared_ptr<FeatureFactory> > to_ins(name, boost::shared_ptr<FeatureFactory>(factory));
  UTIL_THROW_IF2(!registry_.insert(to_ins).second, "Duplicate feature name " << name);
}

namespace
{
class UnknownFeatureException : public util::Exception {};
}

void FeatureRegistry::Construct(const std::string &name, const std::string &line)
{
  Map::iterator i = registry_.find(name);
  UTIL_THROW_IF(i == registry_.end(), UnknownFeatureException, "Feature name " << name << " is not registered.");
  i->second->Create(line);
}

void FeatureRegistry::PrintFF() const
{
	std::cerr << "Available feature functions:" << std::endl;
	Map::const_iterator iter;
	for (iter = registry_.begin(); iter != registry_.end(); ++iter) {
		const string &ffName = iter->first;
		std::cerr << ffName << std::endl;
	}

}

} // namespace Moses
