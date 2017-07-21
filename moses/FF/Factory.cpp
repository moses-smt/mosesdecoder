#include "util/exception.hh"
#include "moses/FF/Factory.h"
#include "moses/StaticData.h"

#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "moses/TranslationModel/PhraseDictionaryMemory.h"
#include "moses/TranslationModel/PhraseDictionaryMultiModel.h"
#include "moses/TranslationModel/PhraseDictionaryMultiModelCounts.h"
#include "moses/TranslationModel/PhraseDictionaryGroup.h"
#include "moses/TranslationModel/PhraseDictionaryScope3.h"
#include "moses/TranslationModel/PhraseDictionaryTransliteration.h"
#include "moses/TranslationModel/PhraseDictionaryDynamicCacheBased.h"
#include "moses/TranslationModel/PhraseDictionaryCache.h"

#include "moses/TranslationModel/RuleTable/PhraseDictionaryOnDisk.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryFuzzyMatch.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryALSuffixArray.h"
#include "moses/TranslationModel/ProbingPT.h"
#include "moses/TranslationModel/PhraseDictionaryMemoryPerSentence.h"
#include "moses/TranslationModel/PhraseDictionaryMemoryPerSentenceOnDemand.h"

#include "moses/FF/LexicalReordering/LexicalReordering.h"

#include "moses/FF/BleuScoreFeature.h"
#include "moses/FF/TargetWordInsertionFeature.h"
#include "moses/FF/SourceWordDeletionFeature.h"
#include "moses/FF/GlobalLexicalModel.h"
#include "moses/FF/GlobalLexicalModelUnlimited.h"
#include "moses/FF/Model1Feature.h"
#include "moses/FF/UnknownWordPenaltyProducer.h"
#include "moses/FF/WordTranslationFeature.h"
#include "moses/FF/TargetBigramFeature.h"
#include "moses/FF/TargetNgramFeature.h"
#include "moses/FF/PhraseBoundaryFeature.h"
#include "moses/FF/PhraseDistanceFeature.h"
#include "moses/FF/PhrasePairFeature.h"
#include "moses/FF/RulePairUnlexicalizedSource.h"
#include "moses/FF/PhraseLengthFeature.h"
#include "moses/FF/DistortionScoreProducer.h"
#include "moses/FF/SparseHieroReorderingFeature.h"
#include "moses/FF/WordPenaltyProducer.h"
#include "moses/FF/InputFeature.h"
#include "moses/FF/PhrasePenalty.h"
#include "moses/FF/OSM-Feature/OpSequenceModel.h"
#include "moses/FF/Dsg-Feature/DsgModel.h"
#include "moses/FF/ControlRecombination.h"
#include "moses/FF/ConstrainedDecoding.h"
#include "moses/FF/SoftSourceSyntacticConstraintsFeature.h"
#include "moses/FF/TargetConstituentAdjacencyFeature.h"
#include "moses/FF/TargetPreferencesFeature.h"
#include "moses/FF/CoveredReferenceFeature.h"
#include "moses/FF/TreeStructureFeature.h"
#include "moses/FF/SoftMatchingFeature.h"
#include "moses/FF/DynamicCacheBasedLanguageModel.h"
#include "moses/FF/SourceGHKMTreeInputMatchFeature.h"
#include "moses/FF/HyperParameterAsWeight.h"
#include "moses/FF/SetSourcePhrase.h"
#include "moses/FF/PhraseOrientationFeature.h"
#include "moses/FF/UnalignedWordCountFeature.h"
#include "CountNonTerms.h"
#include "ReferenceComparison.h"
#include "RuleScope.h"
#include "MaxSpanFreeNonTermSource.h"
#include "NieceTerminal.h"
#include "SpanLength.h"
#include "SyntaxRHS.h"
#include "DeleteRules.h"

#include "moses/FF/ExampleStatelessFF.h"
#include "moses/FF/ExampleStatefulFF.h"
#include "moses/LM/ExampleLM.h"
#include "moses/FF/ExampleTranslationOptionListFeature.h"
#include "moses/LM/BilingualLM.h"
#include "moses/TranslationModel/ExamplePT.h"
#include "moses/Syntax/InputWeightFF.h"
#include "moses/Syntax/RuleTableFF.h"

#include "moses/LM/InMemoryPerSentenceOnDemandLM.h"
#include "moses/FF/EditOps.h"
#include "moses/FF/CorrectionPattern.h"

#ifdef HAVE_VW
#include "moses/FF/VW/VW.h"
#include "moses/FF/VW/VWFeatureContextBigrams.h"
#include "moses/FF/VW/VWFeatureContextBilingual.h"
#include "moses/FF/VW/VWFeatureContextWindow.h"
#include "moses/FF/VW/VWFeatureSourceBagOfWords.h"
#include "moses/FF/VW/VWFeatureSourceBigrams.h"
#include "moses/FF/VW/VWFeatureSourceIndicator.h"
#include "moses/FF/VW/VWFeatureSourcePhraseInternal.h"
#include "moses/FF/VW/VWFeatureSourceSenseWindow.h"
#include "moses/FF/VW/VWFeatureSourceWindow.h"
#include "moses/FF/VW/VWFeatureTargetBigrams.h"
#include "moses/FF/VW/VWFeatureTargetIndicator.h"
#include "moses/FF/VW/VWFeatureSourceExternalFeatures.h"
#include "moses/FF/VW/VWFeatureTargetPhraseInternal.h"
#include "moses/FF/VW/VWFeatureTargetPhraseScores.h"
#endif

#ifdef HAVE_CMPH
#include "moses/TranslationModel/CompactPT/PhraseDictionaryCompact.h"
#endif
#ifdef PT_UG
#include "moses/TranslationModel/UG/mmsapt.h"
#endif

#include "moses/LM/Ken.h"
#include "moses/LM/Reloading.h"
#ifdef LM_IRST
#include "moses/LM/IRST.h"
#endif

#ifdef LM_SRI
#include "moses/LM/SRI.h"
#endif

#ifdef LM_MAXENT_SRI
#include "moses/LM/MaxEntSRI.h"
#endif

#ifdef LM_RAND
#include "moses/LM/Rand.h"
#endif

#ifdef HAVE_SYNLM
#include "moses/SyntacticLanguageModel.h"
#endif

#ifdef LM_NEURAL
#include "moses/LM/NeuralLMWrapper.h"
#include "moses/LM/RDLM.h"
#include "moses/LM/bilingual-lm/BiLM_NPLM.h"
#endif

#ifdef LM_DALM
#include "moses/LM/DALMWrapper.h"
#endif

#ifdef LM_OXLM
#include "moses/LM/oxlm/OxLM.h"
#include "moses/LM/oxlm/SourceOxLM.h"
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

template <class F>
void
FeatureFactory
::DefaultSetup(F *feature)
{
  FeatureFunction::Register(feature);

  StaticData &static_data = StaticData::InstanceNonConst();
  const std::string &featureName = feature->GetScoreProducerDescription();
  std::vector<float> weights = static_data.GetParameter()->GetWeights(featureName);


  if (feature->GetNumScoreComponents()) {
    if (weights.size() == 0) {
      weights = feature->DefaultWeights();
      if (weights.size() == 0) {
        TRACE_ERR("WARNING: No weights specified in config file for FF "
                  << featureName << ". This FF does not supply default values.\n"
                  << "WARNING: Auto-initializing all weights for this FF to 1.0");
        weights.assign(feature->GetNumScoreComponents(),1.0);
      } else {
        VERBOSE(2,"WARNING: No weights specified in config file for FF "
                << featureName << ". Using default values supplied by FF.");
      }
    }
    UTIL_THROW_IF2(weights.size() != feature->GetNumScoreComponents(),
                   "FATAL ERROR: Mismatch in number of features and number "
                   << "of weights for Feature Function " << featureName
                   << " (features: " << feature->GetNumScoreComponents()
                   << " vs. weights: " << weights.size() << ")");
    static_data.SetWeights(feature, weights);
  } else if (feature->IsTuneable())
    static_data.SetWeights(feature, weights);
}

namespace
{

template <class F>
class DefaultFeatureFactory : public FeatureFactory
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

class ReloadingFactory : public FeatureFactory
{
public:
  void Create(const std::string &line) {
    DefaultSetup(ConstructReloadingLM(line));
  }
};

} // namespace

FeatureRegistry::FeatureRegistry()
{
// Feature with same name as class
#define MOSES_FNAME(name) Add(#name, new DefaultFeatureFactory< name >());
// Feature with different name than class.
#define MOSES_FNAME2(name, type) Add(name, new DefaultFeatureFactory< type >());

  MOSES_FNAME2("PhraseDictionaryBinary", PhraseDictionaryTreeAdaptor);
  MOSES_FNAME(PhraseDictionaryOnDisk);
  MOSES_FNAME(PhraseDictionaryMemory);
  MOSES_FNAME(PhraseDictionaryScope3);
  MOSES_FNAME(PhraseDictionaryMultiModel);
  MOSES_FNAME(PhraseDictionaryMultiModelCounts);
  MOSES_FNAME(PhraseDictionaryGroup);
  MOSES_FNAME(PhraseDictionaryALSuffixArray);
  //  MOSES_FNAME(PhraseDictionaryDynSuffixArray);
  MOSES_FNAME(PhraseDictionaryTransliteration);
  MOSES_FNAME(PhraseDictionaryDynamicCacheBased);
  MOSES_FNAME(PhraseDictionaryCache);
  MOSES_FNAME(PhraseDictionaryFuzzyMatch);
  MOSES_FNAME(ProbingPT);
  MOSES_FNAME(PhraseDictionaryMemoryPerSentence);
  MOSES_FNAME(PhraseDictionaryMemoryPerSentenceOnDemand);
  MOSES_FNAME2("RuleTable", Syntax::RuleTableFF);
  MOSES_FNAME2("SyntaxInputWeight", Syntax::InputWeightFF);

  MOSES_FNAME(GlobalLexicalModel);
  //MOSES_FNAME(GlobalLexicalModelUnlimited); This was commented out in the original
  MOSES_FNAME(Model1Feature);
  MOSES_FNAME(SourceWordDeletionFeature);
  MOSES_FNAME(TargetWordInsertionFeature);
  MOSES_FNAME(PhraseBoundaryFeature);
  MOSES_FNAME(PhraseDistanceFeature);
  MOSES_FNAME(PhraseLengthFeature);
  MOSES_FNAME(WordTranslationFeature);
  MOSES_FNAME(TargetBigramFeature);
  MOSES_FNAME(TargetNgramFeature);
  MOSES_FNAME(PhrasePairFeature);
  MOSES_FNAME(RulePairUnlexicalizedSource);
  MOSES_FNAME(LexicalReordering);
  MOSES_FNAME2("Generation", GenerationDictionary);
  MOSES_FNAME(BleuScoreFeature);
  MOSES_FNAME2("Distortion", DistortionScoreProducer);
  MOSES_FNAME2("WordPenalty", WordPenaltyProducer);
  MOSES_FNAME(InputFeature);
  MOSES_FNAME(OpSequenceModel);
  MOSES_FNAME(DesegModel);
  MOSES_FNAME(PhrasePenalty);
  MOSES_FNAME2("UnknownWordPenalty", UnknownWordPenaltyProducer);
  MOSES_FNAME(ControlRecombination);
  MOSES_FNAME(ConstrainedDecoding);
  MOSES_FNAME(CoveredReferenceFeature);
  MOSES_FNAME(SourceGHKMTreeInputMatchFeature);
  MOSES_FNAME(SoftSourceSyntacticConstraintsFeature);
  MOSES_FNAME(TargetConstituentAdjacencyFeature);
  MOSES_FNAME(TargetPreferencesFeature);
  MOSES_FNAME(TreeStructureFeature);
  MOSES_FNAME(SoftMatchingFeature);
  MOSES_FNAME(DynamicCacheBasedLanguageModel);
  MOSES_FNAME(HyperParameterAsWeight);
  MOSES_FNAME(SetSourcePhrase);
  MOSES_FNAME(CountNonTerms);
  MOSES_FNAME(ReferenceComparison);
  MOSES_FNAME(RuleScope);
  MOSES_FNAME(MaxSpanFreeNonTermSource);
  MOSES_FNAME(NieceTerminal);
  MOSES_FNAME(SparseHieroReorderingFeature);
  MOSES_FNAME(SpanLength);
  MOSES_FNAME(SyntaxRHS);
  MOSES_FNAME(PhraseOrientationFeature);
  MOSES_FNAME(UnalignedWordCountFeature);
  MOSES_FNAME(DeleteRules);

  MOSES_FNAME(ExampleStatelessFF);
  MOSES_FNAME(ExampleStatefulFF);
  MOSES_FNAME(ExampleLM);
  MOSES_FNAME(ExampleTranslationOptionListFeature);
  MOSES_FNAME(ExamplePT);

  MOSES_FNAME(InMemoryPerSentenceOnDemandLM);
  MOSES_FNAME(EditOps);
  MOSES_FNAME(CorrectionPattern);

#ifdef HAVE_VW
  MOSES_FNAME(VW);
  MOSES_FNAME(VWFeatureContextBigrams);
  MOSES_FNAME(VWFeatureContextBilingual);
  MOSES_FNAME(VWFeatureContextWindow);
  MOSES_FNAME(VWFeatureSourceBagOfWords);
  MOSES_FNAME(VWFeatureSourceBigrams);
  MOSES_FNAME(VWFeatureSourceIndicator);
  MOSES_FNAME(VWFeatureSourcePhraseInternal);
  MOSES_FNAME(VWFeatureSourceSenseWindow);
  MOSES_FNAME(VWFeatureSourceWindow);
  MOSES_FNAME(VWFeatureTargetBigrams);
  MOSES_FNAME(VWFeatureTargetPhraseInternal);
  MOSES_FNAME(VWFeatureTargetIndicator);
  MOSES_FNAME(VWFeatureSourceExternalFeatures);
  MOSES_FNAME(VWFeatureTargetPhraseScores);
#endif

#ifdef HAVE_CMPH
  MOSES_FNAME(PhraseDictionaryCompact);
#endif
#ifdef PT_UG
  MOSES_FNAME(Mmsapt);
  MOSES_FNAME2("PhraseDictionaryBitextSampling",Mmsapt); // that's an alias for Mmsapt!
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
#ifdef LM_MAXENT_SRI
  MOSES_FNAME2("MaxEntLM", LanguageModelMaxEntSRI);
#endif
#ifdef LM_RAND
  MOSES_FNAME2("RANDLM", LanguageModelRandLM);
#endif
#ifdef LM_NEURAL
  MOSES_FNAME2("NeuralLM", NeuralLMWrapper);
  MOSES_FNAME(RDLM);
  MOSES_FNAME2("BilingualNPLM", BilingualLM_NPLM);
#endif
#ifdef LM_DALM
  MOSES_FNAME2("DALM", LanguageModelDALM);
#endif
#ifdef LM_OXLM
  MOSES_FNAME2("OxLM", OxLM<oxlm::LM>);
  MOSES_FNAME2("OxFactoredLM", OxLM<oxlm::FactoredLM>);
  MOSES_FNAME2("OxFactoredMaxentLM", OxLM<oxlm::FactoredMaxentLM>);
  MOSES_FNAME2("OxSourceFactoredLM", SourceOxLM);
  MOSES_FNAME2("OxTreeLM", OxLM<oxlm::FactoredTreeLM>);
#endif
  Add("ReloadingLM", new ReloadingFactory());
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
  std::vector<std::string> ffs;
  std::cerr << "Available feature functions:" << std::endl;
  Map::const_iterator iter;
  for (iter = registry_.begin(); iter != registry_.end(); ++iter) {
    const std::string &ffName = iter->first;
    ffs.push_back(ffName);
  }

  std::vector<std::string>::const_iterator iterVec;
  std::sort(ffs.begin(), ffs.end());
  for (iterVec = ffs.begin(); iterVec != ffs.end(); ++iterVec) {
    const std::string &ffName = *iterVec;
    std::cerr << ffName << " ";
  }

  std::cerr << std::endl;
}

} // namespace Moses
