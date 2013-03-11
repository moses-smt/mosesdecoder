#ifndef _TranslationTable_h_
#define _TranslationTable_h_

#include "tables-core.h"
#include "FeatureExtractor.h"
#include <vector>
#include <string>
#include <map>
#include <set>

struct TTableTranslation
{
  PSD::AlignmentType m_alignment;
  std::vector<float> m_scores;
};

typedef std::map<size_t, std::map<size_t, TTableTranslation> > DictionaryType;

class TranslationTable
{
public:
  TranslationTable(const std::string &fileName, PSD::IndexType *targetIndex);
  bool SrcExists(const std::string &phrase);

  // get all translations of source phrase, assumes that srcPhrase is known 
  // (throws logic_error otherwise)
  const std::map<size_t, TTableTranslation> &GetTranslations(const std::string &srcPhrase);

private:
  DictionaryType m_ttable;
  PSD::IndexType *m_sourceIndex, *m_targetIndex;

  void AddPhrasePair(const std::string &src, const std::string &tgt,
      const std::vector<float> &scores, const PSD::AlignmentType &align); 
  std::vector<float> GetScores(const std::string &scoreStr);
  PSD::AlignmentType GetAlignment(const std::string &alignStr);

  // add phrase to index (if it does not exist yet), return its ID
  size_t AddPhrase(const std::string &phrase, PSD::IndexType *index);
};

#endif // _TranslationTable_h_
