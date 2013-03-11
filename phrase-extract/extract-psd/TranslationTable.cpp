#include "Util.h"
#include "InputFileStream.h"
#include "TranslationTable.h"
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

TranslationTable::TranslationTable(const string &fileName, PSD::IndexType *targetIndex)
  : m_targetIndex(targetIndex)
{
  m_sourceIndex = new IndexType();
  InputFileStream in(fileName);
  if (! in.good())
    throw runtime_error("error: cannot open " + fileName);
  string line;
  while (getline(in, line)) {
    vector<string> columns = TokenizeMultiCharSeparator(line, " ||| ");
    AddPhrasePair(columns[0], columns[1], GetScores(columns[2]), GetAlignment(columns[3]));
  }
}

bool TranslationTable::SrcExists(const string &phrase)
{
  return m_sourceIndex->left.find(phrase) != m_sourceIndex->left.end();
}

const map<size_t, TTableTranslation> &TranslationTable::GetTranslations(const string &srcPhrase)
{
  IndexType::left_map::const_iterator srcIt = m_sourceIndex->left.find(srcPhrase);
  if (srcIt == m_sourceIndex->left.end())
    throw logic_error("error: unknown source phrase " + srcPhrase);
  DictionaryType::const_iterator it = m_ttable.find(srcIt->second);
  return it->second;
}

//
// private methods
//

void TranslationTable::AddPhrasePair(const std::string &src, const std::string &tgt,
    const std::vector<float> &scores, const PSD::AlignmentType &align)
{
  size_t srcID = AddPhrase(src, m_sourceIndex);
  pair<DictionaryType::iterator, bool> ret = m_ttable.insert(make_pair(srcID, map<size_t, TTableTranslation>()));
  map<size_t, TTableTranslation> &translations = ret.first->second;
  size_t tgtID = AddPhrase(tgt, m_targetIndex);

  TTableTranslation t;
  t.m_alignment = align;
  t.m_scores = scores;
  translations.insert(make_pair(tgtID, t));
}

std::vector<float> TranslationTable::GetScores(const std::string &scoreStr)
{
  return Scan<float>(Tokenize(scoreStr, " "));
}

PSD::AlignmentType TranslationTable::GetAlignment(const std::string &alignStr)
{
  AlignmentType out;
  vector<string> points = Tokenize(alignStr, " ");
  vector<string>::const_iterator it;
  for (it = points.begin(); it != points.end(); it++) {
    vector<size_t> point = Scan<size_t>(Tokenize(*it, "-"));
    if (point.size() != 2)
      throw runtime_error("error: malformed alignment point " + *it);
    out.insert(make_pair(point[0], point[1]));
  }
  return out;
}

size_t TranslationTable::AddPhrase(const string &phrase, IndexType *index)
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
