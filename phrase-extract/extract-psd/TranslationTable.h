#ifndef _TranslationTable_h_
#define _TranslationTable_h_

#include "tables-core.h"
#include "FeatureExtractor.h"
#include <vector>
#include <string>
#include <map>
#include <set>

typedef std::map<std::string, std::vector<PSD::Translation> > DictionaryType;

class TranslationTable
{
public:
  TranslationTable(const std::string &fileName);
  const PSD::TargetIndexType &GetTargetIndex();
  bool SrcExists(const std::string &phrase);

  // get ID of target phrase, set found to true if found, false otherwise
  size_t GetTgtPhraseID(const std::string &phrase, /* out */ bool *found);

  // get all translations of source phrase, assumes that srcPhrase is known 
  // (throws logic_error otherwise)
  const std::vector<PSD::Translation> &GetTranslations(const std::string &srcPhrase);

private:
  DictionaryType m_ttable;
  PSD::TargetIndexType m_targetIndex;

  void AddPhrasePair(const std::string &src, const std::string &tgt,
      const PSD::AlignmentType &align, const std::vector<float> &scores);
  std::vector<float> GetScores(const std::string &scoreStr);
  PSD::AlignmentType GetAlignment(const std::string &alignStr);

  // add phrase to index (if it does not exist yet), return its ID
  size_t AddTargetPhrase(const std::string &phrase);
};

#endif // _TranslationTable_h_
