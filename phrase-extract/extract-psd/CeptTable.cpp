
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
  : m_targetIndex(targetIndex)
{
  m_sourceIndex = new IndexType();
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

const vector<CeptTranslation> &CeptTable::GetTranslations(const string &srcPhrase)
{
	vector<CeptTranslation> out;
	IndexType::left_map::const_iterator srcIt = m_sourceIndex->left.find(srcPhrase);
	if (srcIt == m_sourceIndex->left.end())
    throw logic_error("error: unknown source phrase " + srcPhrase);
	DictionaryType::const_iterator it = m_ctable.find(srcIt->second);

  //source index with several associated targets
  map<size_t, CTableTranslation> translationsForIndex = it->second;

  map<size_t, CTableTranslation> :: const_iterator itrIndex;
  for(itrIndex = translationsForIndex.begin(); itrIndex != translationsForIndex.end(); itrIndex++)
  {
	  CeptTranslation currentTranslation;
	  currentTranslation.m_index = it->first;
	  currentTranslation.m_ctableScores = it->second;
	  out.push_back(currentTranslation);
  }
  return out;
}

//
// private methods
//

void CeptTable::AddCeptPair(const std::string &src, const std::string &tgt,
    const std::vector<float> &scores, const PSD::AlignmentType &align)
{
  size_t srcID = AddCept(src, m_sourceIndex);
  pair<DictionaryType::iterator, bool> ret = m_ttable.insert(make_pair(srcID, map<size_t, TTableTranslation>()));
  map<size_t, TTableTranslation> &translations = ret.first->second;
  size_t tgtID = AddCept(tgt, m_targetIndex);

  TTableTranslation t;
  t.m_alignment = align;
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




