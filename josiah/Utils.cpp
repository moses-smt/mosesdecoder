#include <boost/program_options.hpp>


#include "Utils.h"
#include "Pos.h"
#include "Dependency.h"
#include "DiscriminativeLMFeature.h"
#include "DiscriminativeReorderingFeature.h"
#include "DistortionPenaltyFeature.h"
#include "LanguageModelFeature.h"
#include "LexicalReorderingFeature.h"
#include "MetaFeature.h"
#include "ParenthesisFeature.h"
#include "PhraseFeature.h"
#include "PhraseBoundaryFeature.h"
#include "PhrasePairFeature.h"
#include "PosProjectionFeature.h"
#include "RandomFeature.h"
#include "ReorderingFeature.h"
#include "SourceToTargetRatio.h"
#include "StatelessFeature.h"
#include "SingleStateFeature.h"
#include "WordPenaltyFeature.h"

using namespace std;
namespace po = boost::program_options;

namespace Josiah {
  
  
//  template class SingleStateFeature
//    <Moses::DiscriminativeReorderingFeature,DiscriminativeReorderingState>;
  

  
  void configure_features_from_file(const std::string& filename, FeatureVector& fv, bool disableUWP, FVector& coreWeights){
    //Core features
    fv.push_back(FeatureHandle(new WordPenaltyFeature()));
    if (!disableUWP) {
      fv.push_back(FeatureHandle(new UnknownWordPenaltyFeature()));
    }
    const TranslationSystem& system = StaticData::Instance().GetTranslationSystem
      (TranslationSystem::DEFAULT);
    vector<PhraseDictionaryFeature*> phraseTables = system.GetPhraseDictionaries();
    for (size_t i = 0; i < phraseTables.size(); ++i) {
      fv.push_back(FeatureHandle(new PhraseFeature(phraseTables[i],i)));
    }
    const LMList& lms = system.GetLanguageModels();
    for (LMList::const_iterator i = lms.begin(); i != lms.end(); ++i) {
      fv.push_back(FeatureHandle(new LanguageModelFeature(*i)));
    }
    fv.push_back(FeatureHandle(new DistortionPenaltyFeature()));
    const std::vector<LexicalReordering*>& reorderModels = system.GetReorderModels();
    for (size_t i = 0; i < reorderModels.size(); ++i) {
      fv.push_back(FeatureHandle(new SingleStateFeature(reorderModels[i])));
//      fv.push_back(FeatureHandle(new LexicalReorderingFeature(reorderModels[i],i)));
    }
    
    if (filename.empty()) return;
    std::cerr << "Reading extra features from " << filename << std::endl;
    std::ifstream in(filename.c_str());
    if (!in) {
      throw std::runtime_error("Unable to open feature configuration file");
    }
    // todo: instead of having this function know about all required options of
    // each feature, have features populate options / read variable maps /
    // populate feature_vector using static functions.
    po::options_description desc;
    bool useVerbDiff = false;
    bool useCherry = false;
    bool useDepDist = false;
    bool useSrcTgtRatio = false;
    string posProjectBigramTags;
    size_t posSourceFactor;
    size_t posTargetFactor;
    string parenthesisLefts;
    string parenthesisRights;
    size_t dependencyFactor;
    bool discrimlmBigram;
    string discrimlmVocab;
    FactorType discrimlmFactor;
    string coreWeightFile;
    vector<string> msdConfig;
    vector<string> msdVocab;
    bool phrasePairSourceTarget;
    size_t phrasePairSourceFactor;
    size_t phrasePairTargetFactor;
    size_t randomFeatureCount;
    size_t randomFeatureScaling;
    vector<string> phraseBoundarySourceFactors;
    vector<string> phraseBoundaryTargetFactors;
    desc.add_options()
    ("model1.table", "Model 1 table")
    ("model1.pef_column", "Column containing p(e|f) score")
    ("model1.pfe_column", "Column containing p(f|e) score")
    ("dependency.cherry", po::value<bool>(&useCherry)->default_value(false), "Use Colin Cherry's syntactic cohesiveness feature")
    ("dependency.distortion", po::value<bool>(&useDepDist)->default_value(false), "Use the dependency distortion feature")
    ("dependency.factor", po::value<size_t>(&dependencyFactor)->default_value(1), "Factor representing the dependency tree")
    ("pos.sourcefactor", po::value<size_t>(&posSourceFactor)->default_value(1), "Factor representing the source pos tag")
    ("pos.targetfactor", po::value<size_t>(&posTargetFactor)->default_value(1), "Factor representing the target pos tag")
    ("pos.verbdiff", po::value<bool>(&useVerbDiff)->default_value(false), "Verb difference feature")
    ("pos.projectbigram", po::value<string>(&posProjectBigramTags), 
      "Pos project bigram. Comma separated list of tags, or * for all tags.")
    ("srctgtratio.useFeat", po::value<bool>(&useSrcTgtRatio)->default_value(false), "Use source length to target length ratio feature")
    ("parenthesis.lefts", po::value<std::string>(&parenthesisLefts), "Left parentheses")
    ("parenthesis.rights", po::value<std::string>(&parenthesisRights), "Right parentheses")
    ("discrimlm.vocab", po::value<string>(&discrimlmVocab), "Vocabulary file for discriminative lms")
    ("discrimlm.bigram", po::value<bool>(&discrimlmBigram)->default_value(false), "Use the discriminative lm bigram feature")
    ("discrimlm.factor", po::value<FactorType>(&discrimlmFactor)->default_value(0), "The factor to use for the discuminative lm features")
    ("core.weightfile", po::value<string>(&coreWeightFile), 
        "Weights of core features, if they are to be combined into a single feature")
    ("reordering.msd", po::value<vector<string> >(&msdConfig), 
        "Reordering msd (monotone/swap/discontinuous) feature configuration")
    ("reordering.msdvocab", po::value<vector<string> > (&msdVocab),
        "Vocabularies for msd features. In the form factor_id:source/target:file")
    ("phrasepair.sourcetarget", po::value<bool>(&phrasePairSourceTarget)->zero_tokens()->default_value(false), "Watanabe style phrase pair feature")
    ("phrasepair.sourcefactor", po::value<size_t>(&phrasePairSourceFactor)->default_value(0), "The source factor for the phrase pair feature")
    ("phrasepair.targetfactor", po::value<size_t>(&phrasePairTargetFactor)->default_value(0), "The target factor for the phrase pair feature")
    ("random.numvalues", po::value<size_t>(&randomFeatureCount)->default_value(0),
      "The number of values for the random feature")
    ("random.scaling", po::value<size_t>(&randomFeatureScaling)->default_value(5),
      "The scaling for the random feature")
    ("phraseboundary.sourcefactors", po::value<vector<string> >(&phraseBoundarySourceFactors), "Source factors used in the phrase boundary feature, with optional vocab separated by comma")
    ("phraseboundary.targetfactors", po::value<vector<string> >(&phraseBoundaryTargetFactors), "Target factors used in the phrase boundary feature, with optional vocab separated by comma")
    ;
    
    
    
    po::variables_map vm;
    po::store(po::parse_config_file(in,desc,true), vm);
    notify(vm);
    
    if (!coreWeightFile.empty()) {
      cerr << "Using single feature for core features, loading weights from " << coreWeightFile << endl;
      coreWeights.load(coreWeightFile);
      FeatureHandle metaFeature(new MetaFeature(coreWeights,fv));
      fv.clear();
      fv.push_back(metaFeature);
    }
    
    
 
    if (useVerbDiff) {
      //FIXME: Should be configurable
      fv.push_back(FeatureHandle(new VerbDifferenceFeature(posSourceFactor,posTargetFactor)));
    }
    if (useCherry) {
      fv.push_back(FeatureHandle(new CherrySyntacticCohesionFeature(dependencyFactor)));
    }
    if (useSrcTgtRatio) {
      fv.push_back(FeatureHandle(new SourceToTargetRatioFeature));
    }
    if (useDepDist) {
      fv.push_back(FeatureHandle(new DependencyDistortionFeature(dependencyFactor)));
    }
    if (parenthesisRights.size() > 0 || parenthesisLefts.size() > 0) {
        assert(parenthesisRights.size() == parenthesisLefts.size());
        fv.push_back(FeatureHandle(new ParenthesisFeature(parenthesisLefts,parenthesisRights)));
    }
    if (posProjectBigramTags.size()) {
      fv.push_back(FeatureHandle(new PosProjectionBigramFeature(posSourceFactor,posProjectBigramTags)));
    }
    if (discrimlmBigram) {
      fv.push_back(FeatureHandle(new DiscriminativeLMBigramFeature(discrimlmFactor,discrimlmVocab)));
    }
    if (msdConfig.size()) {
      //TODO
      fv.push_back(FeatureHandle(new SingleStateFeature
        (new Moses::DiscriminativeReorderingFeature(msdConfig,msdVocab))));
    }
    if (phrasePairSourceTarget) {
      fv.push_back(FeatureHandle(
        new PhrasePairFeature(phrasePairSourceFactor,phrasePairTargetFactor)));
    }
    if (randomFeatureCount) {
      fv.push_back(FeatureHandle(
        new StatelessFeatureAdaptor(MosesFeatureHandle(
          new Moses::RandomFeature(randomFeatureCount, randomFeatureScaling)))));
    }
    if (phraseBoundarySourceFactors.size() || phraseBoundaryTargetFactors.size()) 
    {
      FactorList sourceFactorIds;
      vector<string> sourceVocabs;
      FactorList targetFactorIds;
      vector<string> targetVocabs;
      for (size_t i = 0; i < phraseBoundarySourceFactors.size(); ++i) {
        vector<string> tokens = Tokenize(phraseBoundarySourceFactors[i],",");
        assert(tokens.size() <= 2);
        sourceFactorIds.push_back(Scan<FactorType>(tokens[0]));
        if (tokens.size() > 1) {
          sourceVocabs.push_back(tokens[1]);
        } else {
          sourceVocabs.push_back("");
        }
      }
      for (size_t i = 0; i < phraseBoundaryTargetFactors.size(); ++i) {
        vector<string> tokens = Tokenize(phraseBoundaryTargetFactors[i],",");
        assert(tokens.size() <= 2);
        targetFactorIds.push_back(Scan<FactorType>(tokens[0]));
        if (tokens.size() > 1) {
          targetVocabs.push_back(tokens[1]);
        } else {
          targetVocabs.push_back("");
        }
      }
fv.push_back(FeatureHandle(
        new PhraseBoundaryFeature(sourceFactorIds,targetFactorIds,
          sourceVocabs,targetVocabs)));
    }
    in.close();
  }
  
  /*
  bool ValidateAndGetLMFromName(string featsName, LanguageModel* &lm) {
    const ScoreIndexManager& scoreIndexManager = StaticData::Instance().GetScoreIndexManager();
    size_t numScores = scoreIndexManager.GetTotalNumberOfScores();
    
    for (size_t i = 0; i < numScores; ++i) {
      if (scoreIndexManager.GetFeatureName(i) == featsName) {
        const ScoreProducer* scoreProducer = scoreIndexManager.GetScoreProducer(i);
        lm = static_cast<LanguageModel*>(const_cast<ScoreProducer*>(scoreProducer));
        return true;
      }
    }
    return false;  
  }*/
  
}


