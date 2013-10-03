
#include "Util.h"
#include "InputFileStream.h"
#include "CeptTable.h"
#include "FeatureExtractor.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <exception>

using namespace std;
using namespace Moses;
using namespace MosesTraining;
using namespace PSD;
using namespace boost::bimaps;

CeptTable::CeptTable(const string &fileName)
{
  m_sourceIndex = new IndexType();
  m_targetIndex = new IndexType();
  InputFileStream in(fileName);
  if (! in.good())
    throw runtime_error("error: cannot open " + fileName);
  string line;
  while (getline(in, line)) {
    vector<string> columns = TokenizeMultiCharSeparator(line, " ||| ");
    //can modify to whatever...
    AddCeptPair(columns[0], columns[1], GetScores(columns[2]));
  }
}

vector<CeptTranslation> CeptTable::GetTranslations(const string &srcPhrase)
{
	vector<CeptTranslation> out;
	IndexType::left_map::const_iterator srcIt = m_sourceIndex->left.find(srcPhrase);
	if (srcIt == m_sourceIndex->left.end())
    throw logic_error("error: unknown source phrase " + srcPhrase);
	CeptDictionaryType::const_iterator it = m_ctable.find(srcIt->second);

  //source index with several associated targets
  const map<size_t, CTableTranslation> &translationsForIndex = it->second;

  map<size_t, CTableTranslation>::const_iterator idxIt;
  for(idxIt = translationsForIndex.begin(); idxIt != translationsForIndex.end(); idxIt++)
  {
	  CeptTranslation currentTranslation;
	  currentTranslation.m_index = idxIt->first;
	  currentTranslation.m_scores = idxIt->second.m_scores;
	  out.push_back(currentTranslation);
  }
  return out;
}

//
// private methods
//

void CeptTable::AddCeptPair(const std::string &src, const std::string &tgt,
    const std::vector<float> &scores)
{
  size_t srcID = AddCept(src, m_sourceIndex);
  pair<CeptDictionaryType::iterator, bool> ret = m_ctable.insert(make_pair(srcID, map<size_t, CTableTranslation>()));
  map<size_t, CTableTranslation> &translations = ret.first->second;
  size_t tgtID = AddCept(tgt, m_targetIndex);

  CTableTranslation t;
  t.m_scores = scores;
  translations.insert(make_pair(tgtID, t));
}

std::vector<float> CeptTable::GetScores(const std::string &scoreStr)
{
  return Scan<float>(Tokenize(scoreStr, " "));
}

size_t CeptTable::AddCept(const string &phrase, IndexType *index)
{
  size_t id;
  IndexType::left_map::const_iterator it = index->left.find(phrase);
  if (it != index->left.end()) {
    id = it->second;
  } else {
    id = index->size() + 1; // index is one-based because of VW
    index->left.insert(IndexType::left_map::value_type(phrase, id));
  }
  return id;
}

size_t CeptTable::GetTgtPhraseID(const string &phrase, /* out */ bool *found)
{
  *found = false;
  IndexType::left_map::const_iterator it = m_targetIndex->left.find(phrase);
  if (it != m_targetIndex->left.end()) {
    *found = true;
    return it->second;
  } else {
    return 0; // user must test value of found!
  }
}

const string &CeptTable::GetTgtString(size_t tgtIdx)
{
  IndexType::right_map::const_iterator it = m_targetIndex->right.find(tgtIdx);
  if (it != m_targetIndex->right.end()) {
    return it->second;
  } else {
    throw runtime_error("Index does not contain requested value");
  }
}
