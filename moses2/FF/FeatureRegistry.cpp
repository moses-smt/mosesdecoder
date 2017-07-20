#include "FeatureRegistry.h"

#include "../TranslationModel/Memory/PhraseTableMemory.h"
#include "../TranslationModel/ProbingPT.h"
#include "../TranslationModel/UnknownWordPenalty.h"
#include "../TranslationModel/Transliteration.h"

#include "../LM/KENLM.h"
#include "../LM/KENLMBatch.h"
#include "../LM/LanguageModel.h"
#include "../LM/GPULM.h"

#include "Distortion.h"
#include "LexicalReordering/LexicalReordering.h"
#include "PhrasePenalty.h"
#include "WordPenalty.h"
#include "OSM/OpSequenceModel.h"

#include "ExampleStatefulFF.h"
#include "ExampleStatelessFF.h"

using namespace std;


namespace Moses2
{
FeatureRegistry FeatureRegistry::s_instance;

template<class F>
class DefaultFeatureFactory: public FeatureFactory
{
public:
  FeatureFunction *Create(size_t startInd, const std::string &line) const {
    return new F(startInd, line);
  }
};

////////////////////////////////////////////////////////////////////
class KenFactory: public FeatureFactory
{
public:
  FeatureFunction *Create(size_t startInd, const std::string &line) const {
    return ConstructKenLM(startInd, line);
  }
};

////////////////////////////////////////////////////////////////////
FeatureRegistry::FeatureRegistry()
{
  // Feature with same name as class
#define MOSES_FNAME(name) Add(#name, new DefaultFeatureFactory< name >());
  // Feature with different name than class.
#define MOSES_FNAME2(name, type) Add(name, new DefaultFeatureFactory< type >());

  MOSES_FNAME2("PhraseDictionaryMemory", PhraseTableMemory);
  MOSES_FNAME(ProbingPT);
  MOSES_FNAME2("PhraseDictionaryTransliteration", Transliteration);
  MOSES_FNAME(UnknownWordPenalty);

  Add("KENLM", new KenFactory());

  MOSES_FNAME(KENLMBatch);
  MOSES_FNAME(GPULM);

  MOSES_FNAME(LanguageModel);

  MOSES_FNAME(Distortion);
  MOSES_FNAME(LexicalReordering);
  MOSES_FNAME(PhrasePenalty);
  MOSES_FNAME(WordPenalty);
  MOSES_FNAME(OpSequenceModel);

  MOSES_FNAME(ExampleStatefulFF);
  MOSES_FNAME(ExampleStatelessFF);
}

FeatureRegistry::~FeatureRegistry()
{

}

void FeatureRegistry::Add(const std::string &name, FeatureFactory *factory)
{
  std::pair<std::string, boost::shared_ptr<FeatureFactory> > to_ins(name,
      boost::shared_ptr<FeatureFactory>(factory));
  if (!registry_.insert(to_ins).second) {
    cerr << "Duplicate feature name " << name << endl;
    abort();
  }
}

FeatureFunction *FeatureRegistry::Construct(size_t startInd,
    const std::string &name, const std::string &line) const
{
  Map::const_iterator i = registry_.find(name);
  if (i == registry_.end()) {
    cerr << "Feature name " << name << " is not registered.";
    abort();
  }
  FeatureFactory *fact = i->second.get();
  FeatureFunction *ff = fact->Create(startInd, line);
  return ff;
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

}

