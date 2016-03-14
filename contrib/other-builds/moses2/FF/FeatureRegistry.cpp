#include "FeatureRegistry.h"

#include "../TranslationModel/PhraseTableMemory.h"
#include "../TranslationModel/ProbingPT.h"
#include "../TranslationModel/UnknownWordPenalty.h"

#include "../LM/KENLM.h"
#include "../LM/LanguageModel.h"

#include "../FF/Distortion.h"
#include "../FF/LexicalReordering.h"
#include "../FF/PhrasePenalty.h"
#include "../FF/WordPenalty.h"

#include "../FF/SkeletonStatefulFF.h"
#include "../FF/SkeletonStatelessFF.h"

using namespace std;

namespace Moses2
{

FeatureRegistry::FeatureRegistry()
{
  // Feature with same name as class
  #define MOSES_FNAME(name) Add(#name, new DefaultFeatureFactory< name >());
  // Feature with different name than class.
  #define MOSES_FNAME2(name, type) Add(name, new DefaultFeatureFactory< type >());

  MOSES_FNAME2("PhraseDictionaryMemory", PhraseTableMemory);
  MOSES_FNAME(ProbingPT);
  MOSES_FNAME(UnknownWordPenalty);

  MOSES_FNAME(KENLM);
  MOSES_FNAME(LanguageModel);

  MOSES_FNAME(Distortion);
  MOSES_FNAME(LexicalReordering);
  MOSES_FNAME(PhrasePenalty);
  MOSES_FNAME(WordPenalty);
  MOSES_FNAME(SkeletonStatefulFF);
  MOSES_FNAME(SkeletonStatelessFF);
}

FeatureRegistry::~FeatureRegistry()
{

}

void FeatureRegistry::Add(const std::string &name, FeatureFactory *factory)
{
  std::pair<std::string, boost::shared_ptr<FeatureFactory> > to_ins(name, boost::shared_ptr<FeatureFactory>(factory));
  if (!registry_.insert(to_ins).second) {
	  cerr << "Duplicate feature name " << name << endl;
	  abort();
  }
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

