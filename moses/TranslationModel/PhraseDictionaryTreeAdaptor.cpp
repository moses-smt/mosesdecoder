// $Id$

#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include <sys/stat.h>
#include <algorithm>
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

using namespace std;

namespace Moses
{
/*************************************************************
	function definitions of the interface class
	virtually everything is forwarded to the implementation class
*************************************************************/

PhraseDictionaryTreeAdaptor::
PhraseDictionaryTreeAdaptor(const std::string &line)
  : PhraseDictionary("PhraseDictionaryTreeAdaptor", line)
{
  imp = new PDTAimp(this,m_numInputScores);
}

PhraseDictionaryTreeAdaptor::~PhraseDictionaryTreeAdaptor()
{
  imp->CleanUp();
  delete imp;
}

bool PhraseDictionaryTreeAdaptor::InitDictionary()
{
  const StaticData &staticData = StaticData::Instance();

  vector<float> weight = staticData.GetWeights(this);
  const LMList &languageModels = staticData.GetLMList();

  if(m_numScoreComponents!=weight.size()) {
    std::stringstream strme;
    strme << "ERROR: mismatch of number of scaling factors: "<<weight.size()
          <<" "<<m_numScoreComponents<<"\n";
    UserMessage::Add(strme.str());
    return false;
  }

  imp->Create(m_input, m_output, m_filePath, weight, languageModels);
  return true;
}

void PhraseDictionaryTreeAdaptor::InitializeForInput(InputType const& source)
{
  imp->CleanUp();
  // caching only required for confusion net
  if(ConfusionNet const* cn=dynamic_cast<ConfusionNet const*>(&source))
    imp->CacheSource(*cn);
}

TargetPhraseCollection const*
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollection(Phrase const &src) const
{
  return imp->GetTargetPhraseCollection(src);
}

TargetPhraseCollection const*
PhraseDictionaryTreeAdaptor::GetTargetPhraseCollection(InputType const& src,WordsRange const &range) const
{
  if(imp->m_rangeCache.empty()) {
    return imp->GetTargetPhraseCollection(src.GetSubString(range));
  } else {
    return imp->m_rangeCache[range.GetStartPos()][range.GetEndPos()];
  }
}

void PhraseDictionaryTreeAdaptor::EnableCache()
{
  imp->useCache=1;
}
void PhraseDictionaryTreeAdaptor::DisableCache()
{
  imp->useCache=0;
}

}
