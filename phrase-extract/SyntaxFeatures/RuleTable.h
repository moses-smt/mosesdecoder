#ifndef _RuleTable_h_
#define _RuleTable_h_

#include "tables-core.h"
#include "psd/FeatureExtractor.h"
#include <vector>
#include <string>
#include <map>
#include <set>

typedef std::map<std::string, std::vector<PSD::ChartTranslation> > DictionaryType;

class RuleTable
{
public:
  RuleTable(const std::string &fileName);
  const PSD::TargetIndexType &GetTargetIndex();
  bool SrcExists(const std::string &phrase);

  // get ID of target phrase, set found to true if found, false otherwise
  size_t GetTgtPhraseID(const std::string &phrase, /* out */ bool *found);

  // get all translations of source phrase, assumes that srcPhrase is known
  // (throws logic_error otherwise)
  const std::vector<PSD::ChartTranslation> &GetTranslations(const std::string &srcPhrase);

private:
  DictionaryType m_ttable;
  PSD::TargetIndexType m_targetIndex;

void AddRulePair(const std::string &src, const std::string &tgt,
    const std::vector<long double> &scores, const PSD::AlignmentType &termAlign,
    const PSD::AlignmentType &nonTermAlign);
  std::vector<long double> GetScores(const std::string &scoreStr);
  PSD::AlignmentType GetTermAlignment(const std::string &alignStr, const std::string &targetStr);
  PSD::AlignmentType GetNonTermAlignment(const std::string &alignStr, const std::string &targetStr, const std::string &sourceStr);

  // add phrase to index (if it does not exist yet), return its ID
  size_t AddTargetPhrase(const std::string &phrase);
};

#endif // _RuleTable_h_
