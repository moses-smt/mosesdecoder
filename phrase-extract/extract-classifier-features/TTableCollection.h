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

struct IndexedTranslation : public Classifier::Translation
{
  size_t m_index;

  // silly helper method 
  static std::vector<Translation> Slice(const std::vector<IndexedTranslation> &in)
  {
    std::vector<Translation> out(in.size());
    for (size_t i = 0; i < in.size(); i++) out[i] = in[i];
    return out;
  }
};

class TTableCollection
{
public:
  TTableCollection(const std::string &ttableArg);

  inline const IndexType *GetTargetIndex() { return m_targetIndex; }

  // get translations of source phrase in all phrase tables
  std::vector<IndexedTranslation> GetAllTranslations(const std::string &srcPhrase, bool intersection = false);

  // get ID of target phrase, set found to true if found, false otherwise
  size_t GetTgtPhraseID(const std::string &phrase, /* out */ bool *found);

  bool SrcExists(const std::string &srcPhrase);

private:
  IndexType *m_targetIndex;
  std::vector<TTableInfo> m_ttables;
};

#endif // _TTableCollection_h_
