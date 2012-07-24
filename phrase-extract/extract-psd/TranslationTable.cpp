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

TranslationTable::TranslationTable(const string &fileName)
{
  InputFileStream in(fileName);
  if (! in.good())
    throw runtime_error("error: cannot open " + fileName);
  string line;
  while (getline(in, line)) {
    vector<string> columns = TokenizeMultiCharSeparator(line, " ||| ");
    AddPhrasePair(columns[0], columns[1], GetAlignment(columns[4]), GetScores(columns[3]));
  }
}

const TargetIndexType &TranslationTable::GetTargetIndex()
{
  return m_targetIndex;
}

bool TranslationTable::SrcExists(const string &phrase)
{
  return m_ttable.find(phrase) != m_ttable.end();
}

size_t TranslationTable::GetTgtPhraseID(const string &phrase, /* out */ bool *found)
{
  *found = false;
  TargetIndexType::left_map::const_iterator it = m_targetIndex.left.find(phrase);
  if (it != m_targetIndex.left.end()) {
    *found = true;
    return it->second;
  } else {
    return 0; // user must test value of found!
  }
}

const vector<Translation> &TranslationTable::GetTranslations(const string &srcPhrase)
{
  DictionaryType::const_iterator it = m_ttable.find(srcPhrase);
  if (it == m_ttable.end())
    throw logic_error("error: unknown source phrase " + srcPhrase);
  return it->second;
}

//
// private methods
//

void TranslationTable::AddPhrasePair(const std::string &src, const std::string &tgt,
    const PSD::AlignmentType &align, const std::vector<float> &scores)
{
  pair<DictionaryType::iterator, bool> ret = m_ttable.insert(make_pair(src, vector<Translation>()));
  vector<Translation> &translations = ret.first->second;
  size_t tgtID = AddTargetPhrase(tgt);

  Translation t;
  t.m_index = tgtID;
  t.m_alignment = align;
  t.m_scores = scores;
  translations.push_back(t);
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

size_t TranslationTable::AddTargetPhrase(const string &phrase)
{
  bool found;
  size_t id = GetTgtPhraseID(phrase, &found);
  if (! found) {
    id = m_targetIndex.size() + 1; // index is one-based because of VW
    m_targetIndex.left.insert(TargetIndexType::left_map::value_type(phrase, id));
  }
  return id;
}
