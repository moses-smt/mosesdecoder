#ifndef _TTableCollection_h_
#define _TTableCollection_h_

#include "tables-core.h"
#include "FeatureExtractor.h"
#include "TranslationTable.h"
#include <vector>
#include <string>
#include <map>
#include <set>

struct TTableInfo
{
  std::string m_id;
  TranslationTable *m_ttable;
};

class TTableCollection
{
public:
  TTableCollection(const std::string &ttableArg);

  inline const PSD::IndexType *GetTargetIndex() { return m_targetIndex; }

  // get translations of source phrase in all phrase tables
  std::vector<PSD::Translation> GetAllTranslations(const std::string &srcPhrase);

  // get ID of target phrase, set found to true if found, false otherwise
  size_t GetTgtPhraseID(const std::string &phrase, /* out */ bool *found);

  bool SrcExists(const std::string &srcPhrase);

private:
  PSD::IndexType *m_targetIndex;
  std::vector<TTableInfo> m_ttables;
};

#endif // _TTableCollection_h_
