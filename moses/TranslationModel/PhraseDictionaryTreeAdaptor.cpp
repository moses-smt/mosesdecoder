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
#include "util/exception.hh"

using namespace std;

namespace Moses
{
/*************************************************************
	function definitions of the interface class
	virtually everything is forwarded to the implementation class
*************************************************************/

PhraseDictionaryTreeAdaptor::
PhraseDictionaryTreeAdaptor(const std::string &line)
  : PhraseDictionary(line)
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

  ReduceCache();

  PDTAimp *obj = new PDTAimp(this);

  vector<float> weight = staticData.GetWeights(this);
  if(m_numScoreComponents!=weight.size()) {
    std::stringstream strme;
    UTIL_THROW2("ERROR: mismatch of number of scaling factors: " << weight.size()
                << " " << m_numScoreComponents);
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
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollectionNonCacheLEGACY(Phrase const &src) const
{
  const TargetPhraseCollection *ret = GetImplementation().GetTargetPhraseCollection(src);
  return ret;
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
  UTIL_THROW_IF2(dict == NULL, "Dictionary object not yet created for this thread");
  return *dict;
}

const PDTAimp& PhraseDictionaryTreeAdaptor::GetImplementation() const
{
  PDTAimp* dict;
  dict = m_implementation.get();
  UTIL_THROW_IF2(dict == NULL, "Dictionary object not yet created for this thread");
  return *dict;
}

// legacy
const TargetPhraseCollectionWithSourcePhrase*
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollectionLEGACY(InputType const& src,WordsRange const &range) const
{
  if(GetImplementation().m_rangeCache.empty()) {
    const TargetPhraseCollectionWithSourcePhrase *tpColl = GetImplementation().GetTargetPhraseCollection(src.GetSubString(range));
    return tpColl;
  } else {
    const TargetPhraseCollectionWithSourcePhrase *tpColl = GetImplementation().m_rangeCache[range.GetStartPos()][range.GetEndPos()];
    return tpColl;
  }
}

}
