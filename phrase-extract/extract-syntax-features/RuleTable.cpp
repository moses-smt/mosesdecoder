#include "Util.h"
#include "InputFileStream.h"
#include "RuleTable.h"
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

RuleTable::RuleTable(const string &fileName)
{
  InputFileStream in(fileName);
  if (! in.good())
    throw runtime_error("error: cannot open " + fileName);
  string line;
  while (getline(in, line)) {
    vector<string> columns = TokenizeMultiCharSeparator(line, " ||| ");
    AddPhrasePair(columns[0], columns[1], GetScores(columns[2]), GetAlignment(columns[3],columns[1]));
  }
}

const TargetIndexType &RuleTable::GetTargetIndex()
{
  return m_targetIndex;
}

bool RuleTable::SrcExists(const string &phrase)
{
  return m_ttable.find(phrase) != m_ttable.end();
}

size_t RuleTable::GetTgtPhraseID(const string &phrase, /* out */ bool *found)
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

const vector<Translation> &RuleTable::GetTranslations(const string &srcPhrase)
{
  DictionaryType::const_iterator it = m_ttable.find(srcPhrase);
  if (it == m_ttable.end())
    throw logic_error("error: unknown source phrase " + srcPhrase);
  return it->second;
}

//
// private methods
//

void RuleTable::AddPhrasePair(const std::string &src, const std::string &tgt,
    const std::vector<float> &scores, const PSD::AlignmentType &align)
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

std::vector<float> RuleTable::GetScores(const std::string &scoreStr)
{
  return Scan<float>(Tokenize(scoreStr, " "));
}

PSD::AlignmentType RuleTable::GetAlignment(const std::string &alignStr, const std::string &targetStr)
{
  AlignmentType out;
  vector<string> points = Tokenize(alignStr, " ");

    //check which alignments are form non-terminals by looking at target side
    //store target side of alignment between non-terminals
    vector<string> targetAligns;
    vector<string> targetToken = Tokenize(targetStr, " ");
    //look for non-terminals in target side
    vector<string> :: iterator itr_targets;
    std::string nonTermString = "[X][X]";

    //cerr << "TARGET STRING : " << targetStr << endl;

    for(itr_targets = targetToken.begin();itr_targets != targetToken.end(); itr_targets++)
    {
        //cerr << "TARGET TOKEN : " << *itr_targets << endl;

        size_t found = (*itr_targets).find(nonTermString);
        if(found != string::npos)
        {
            CHECK((*itr_targets).size() > 1);
            string indexString = (*itr_targets).substr(6,1);
            //cerr << "INDEX STRING" << indexString << endl;
            targetAligns.push_back(indexString);
        }
     }
      vector<string>::const_iterator alignIt;
      for (alignIt = points.begin(); alignIt != points.end(); alignIt++) {
        vector<string> point = Tokenize(*alignIt, "-");
        bool isNonTerm = false;
        if (point.size() == 2) {
          //damt_hiero : NOTE : maybe inefficient change if hurts performance
          //WRONG : TODO : REMOVE THE i-th index STRING INDEX
          for(itr_targets = targetAligns.begin(); itr_targets != targetAligns.end(); itr_targets++)
          {
              if(point.back()==(*itr_targets)) isNonTerm = true;
          }
          if(!isNonTerm)
          {
              //cerr << "INSERTING : " << point[0] <<  " : " << point[1] << endl;
              out.insert(make_pair(Scan<size_t>(point[0]), Scan<size_t>(point[1])));
          }
        }
        else{throw runtime_error("error: malformed alignment point " + *alignIt);}
      }
    return out;
}

size_t RuleTable::AddTargetPhrase(const string &phrase)
{
  bool found;
  size_t id = GetTgtPhraseID(phrase, &found);
  if (! found) {
    id = m_targetIndex.size() + 1; // index is one-based because of VW
    m_targetIndex.left.insert(TargetIndexType::left_map::value_type(phrase, id));
  }
  return id;
}
