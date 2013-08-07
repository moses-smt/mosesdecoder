// $Id$

#include <sys/stat.h>
#include <algorithm>
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "moses/TranslationModel/PhraseDictionaryTree.h"
#include "moses/Phrase.h"
#include "moses/FactorCollection.h"
#include "moses/InputFileStream.h"
#include "moses/InputType.h"
#include "moses/ConfusionNet.h"
#include "moses/Sentence.h"
#include "moses/StaticData.h"
#include "moses/UniqueObject.h"
#include "moses/PDTAimp.h"
#include "moses/UserMessage.h"
#include "util/check.hh"

using namespace std;

namespace Moses
{
/*************************************************************
	function definitions of the interface class
	virtually everything is forwarded to the implementation class
*************************************************************/

PhraseDictionaryTreeAdaptor::
PhraseDictionaryTreeAdaptor(const std::string &line)
  : PhraseDictionary("PhraseDictionaryBinary", line)
{
  ReadParameters();
}

PhraseDictionaryTreeAdaptor::~PhraseDictionaryTreeAdaptor()
{
}
void PhraseDictionaryTreeAdaptor::Load()
{
  SetFeaturesToApply();
}

void PhraseDictionaryTreeAdaptor::InitializeForInput(InputType const& source)
{
  const StaticData &staticData = StaticData::Instance();

  PDTAimp *obj = new PDTAimp(this);

  vector<float> weight = staticData.GetWeights(this);
  if(m_numScoreComponents!=weight.size()) {
    std::stringstream strme;
    strme << "ERROR: mismatch of number of scaling factors: "<<weight.size()
          <<" "<<m_numScoreComponents<<"\n";
    UserMessage::Add(strme.str());
    abort();
  }

  obj->Create(m_input, m_output, m_filePath, weight);

  obj->CleanUp();
  // caching only required for confusion net
  if(ConfusionNet const* cn=dynamic_cast<ConfusionNet const*>(&source))
    obj->CacheSource(*cn);

  m_implementation.reset(obj);
}

void PhraseDictionaryTreeAdaptor::CleanUpAfterSentenceProcessing(InputType const& source)
{
  PDTAimp &obj = GetImplementation();
  obj.CleanUp();
}

TargetPhraseCollection const*
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollection(Phrase const &src) const
{
  return GetImplementation().GetTargetPhraseCollection(src);
}

void PhraseDictionaryTreeAdaptor::EnableCache()
{
  GetImplementation().useCache=1;
}
void PhraseDictionaryTreeAdaptor::DisableCache()
{
  GetImplementation().useCache=0;
}

PDTAimp& PhraseDictionaryTreeAdaptor::GetImplementation()
{
  PDTAimp* dict;
  dict = m_implementation.get();
  CHECK(dict);
  return *dict;
}

const PDTAimp& PhraseDictionaryTreeAdaptor::GetImplementation() const
{
  PDTAimp* dict;
  dict = m_implementation.get();
  CHECK(dict);
  return *dict;
}

// legacy
std::pair<const TargetPhraseCollection*, std::vector<Phrase> >
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollectionLegacy(InputType const& src,WordsRange const &range) const
{
  if(GetImplementation().m_rangeCache.empty()) {
    const TargetPhraseCollection *tpColl = GetImplementation().GetTargetPhraseCollection(src.GetSubString(range));
    std::vector<Phrase> sourPhrases;
    std::pair<const TargetPhraseCollection*, std::vector<Phrase> > ret(tpColl, sourPhrases);
    return ret;
  } else {
	  const TargetPhraseCollection *tpColl = GetImplementation().m_rangeCache[range.GetStartPos()][range.GetEndPos()];
	  std::vector<Phrase> sourPhrases;
	  std::pair<const TargetPhraseCollection*, std::vector<Phrase> > ret(tpColl, sourPhrases);
      return ret;
  }
}

}
