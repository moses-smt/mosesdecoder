// vim:tabstop=2
#include "SkeletonPT.h"

using namespace std;

namespace Moses
{
SkeletonPT::SkeletonPT(const std::string &line)
  : PhraseDictionary("SkeletonPT", line)
{
  ReadParameters();
}

void 
SkeletonPT::
GetTargetPhraseCollectionBatch(const InputPathList &phraseDictionaryQueue) const
{
#if 0
  InputPathList::const_iterator iter;
  for (iter = phraseDictionaryQueue.begin(); iter != phraseDictionaryQueue.end(); ++iter) {
    InputPath &inputPath = **iter;
  }
#endif
}

ChartRuleLookupManager* 
SkeletonPT::
CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&)
{

}

TO_STRING_BODY(SkeletonPT);

// friend
ostream& operator<<(ostream& out, const SkeletonPT& phraseDict)
{
  return out;
}

}
