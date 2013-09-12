/*
 * CeptTable.h
 *
 *  Created on: 12.09.2013
 *      Author: braunefe
 */

#ifndef CEPTTABLE_H_
#define CEPTTABLE_H_

#include "tables-core.h"
#include "DWLFeatureExtractor.h"
#include <vector>
#include <string>
#include <map>
#include <set>

struct CTableTranslation
{
  std::vector<float> m_scores;
};

typedef std::map<size_t, std::map<size_t, CTableTranslation> > CeptDictionaryType;

class CeptTable
{
public:

 CeptTable(const std::string &fileName);

 const PSD::IndexType *GetTargetIndex() { return m_targetIndex; }
 bool SrcExists(const std::string &phrase)
 {
   return m_sourceIndex->left.find(phrase) != m_sourceIndex->left.end();
 }

  // get all translations of source phrase, assumes that srcPhrase is known
  // (throws logic_error otherwise)
  std::vector<PSD::CeptTranslation> GetTranslations(const std::string &srcCept);

  size_t GetTgtPhraseID(const std::string &phrase, /* out */ bool *found);

private:
  CeptDictionaryType m_ctable;
  PSD::IndexType *m_sourceIndex, *m_targetIndex;

  void AddCeptPair(const std::string &src, const std::string &tgt,
      const std::vector<float> &scores);
  std::vector<float> GetScores(const std::string &scoreStr);

  // add phrase to index (if it does not exist yet), return its ID
  size_t AddCept(const std::string &phrase, PSD::IndexType *index);

};


#endif /* CEPTTABLE_H_ */
