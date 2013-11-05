#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "RuleTable.h"
#include "psd/FeatureExtractor.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <exception>

using namespace std;
using namespace Moses;
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
    //std::cerr << "In rule table, adding rule pair : X" << columns[0] << "X : X" << columns[1] << "X" << std::endl;
    AddRulePair(columns[0], columns[1], GetScores(columns[2]), GetTermAlignment(columns[3],columns[1]), GetNonTermAlignment(columns[3],columns[1],columns[0]));
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
  //std::cerr << "Looking for phrase : " << phrase << std::endl;
  TargetIndexType::left_map::const_iterator it = m_targetIndex.left.find(phrase);
  if (it != m_targetIndex.left.end()) {
    *found = true;
    return it->second;
  } else {
    return 0; // user must test value of found!
  }
}

const vector<ChartTranslation> &RuleTable::GetTranslations(const string &srcPhrase)
{
  DictionaryType::const_iterator it = m_ttable.find(srcPhrase);
  if (it == m_ttable.end())
    throw logic_error("error: unknown source phrase " + srcPhrase);
  return it->second;
}

//
// private methods
//

void RuleTable::AddRulePair(const std::string &src, const std::string &tgt,
    const std::vector<long double> &scores, const PSD::AlignmentType &termAlign, const PSD::AlignmentType &nonTermAlign)
{

  //std::cerr << "Adding rule pair "<< src << " : "<< tgt << std::endl;
  pair<DictionaryType::iterator, bool> ret = m_ttable.insert(make_pair(src, vector<ChartTranslation>()));
  vector<ChartTranslation> &translations = ret.first->second;
  size_t tgtID = AddTargetPhrase(tgt);

  ChartTranslation t;
  t.m_index = tgtID;
  t.m_termAlignment = termAlign;
  t.m_nonTermAlignment = nonTermAlign;
  t.m_scores = scores;
  translations.push_back(t);
}

std::vector<long double> RuleTable::GetScores(const std::string &scoreStr)
{
  return Scan<long double>(Tokenize(scoreStr, " "));
}

PSD::AlignmentType RuleTable::GetTermAlignment(const std::string &alignStr, const std::string &targetStr)
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

PSD::AlignmentType RuleTable::GetNonTermAlignment(const std::string &alignStr, const std::string &targetStr, const std::string &sourceStr)
{
  AlignmentType out;
  vector<string> points = Tokenize(alignStr, " ");

  //cerr << "READING SOURCE : " << sourceStr << endl;
  //cerr << "READING TARGET : " << targetStr << endl;

    //check which alignments are form non-terminals by looking at target side
    //store target side of alignment between non-terminals
    std::map<size_t,size_t> targetAligns;
    std::map<size_t,size_t> sourceAligns;
    vector<string> targetToken = Tokenize(targetStr, " ");
    vector<string> sourceToken = Tokenize(sourceStr, " ");
    //look for non-terminals in target side
    vector<string> :: iterator itr_targets;
    vector<string> :: iterator itr_source;
    std::string nonTermString = "[X][X]";

    size_t sourceCounter = 0;
    size_t targetCounter = 0;
    size_t sourceNonTermCounter = 0;
    size_t targetNonTermCounter = 0;

    //For targets, alignment with source is encoded in target
    for(itr_targets = targetToken.begin();itr_targets != targetToken.end(); itr_targets++)
    {
        size_t found = (*itr_targets).find(nonTermString);
        if(found != string::npos)
        {
            targetAligns.insert(make_pair(targetCounter,targetNonTermCounter));
            //std::cerr << "TARGET ALIGN : " << targetCounter << " : " << targetNonTermCounter << std::endl;
            targetNonTermCounter++;
        }
        targetCounter++;
     }

     for(itr_source = sourceToken.begin();itr_source != sourceToken.end(); itr_source++)
    {
        size_t found = (*itr_source).find(nonTermString);
        if(found != string::npos)
        {
            sourceAligns.insert(make_pair(sourceCounter,sourceNonTermCounter));
            //std::cerr << "SOURCE ALIGN : " << sourceCounter << " : " << sourceNonTermCounter << std::endl;
            sourceNonTermCounter++;
        }
        sourceCounter++;
     }

      vector<string>::const_iterator alignIt;

      //If there are no non-terminals output as it is
      if(sourceAligns.size() == 0) return out;

      //Assumption : the first alignments are between non-terminals
      CHECK(sourceAligns.size() == targetAligns.size());
      size_t numberOfNonTerms = sourceAligns.size();

      for (alignIt = points.begin(); alignIt != points.end(); alignIt++) {
        vector<string> point = Tokenize(*alignIt, "-");
        //look at size of source and target non term vectors
        if (point.size() == 2) {

        //std::cerr << "CHECKING FOR POINT ON SOURCE : " << point[0] << " : " << std::endl;
        //std::cerr << "CHECKING FOR POINT ON TARGET : " << point[1] << " : " << std::endl;
        CHECK(sourceAligns.find(Scan<size_t>(point[0])) != sourceAligns.end());
        CHECK(targetAligns.find(Scan<size_t>(point[1])) != targetAligns.end());

        //cerr << "INSERTING : " << point[0] <<  " : " << point[1] << endl;
        out.insert( make_pair(sourceAligns.find(Scan<size_t>(point[0]))->second,
                               targetAligns.find(Scan<size_t>(point[1]))->second ) );
        //std:cerr << "INSERTED PAIR : " << sourceAligns.find(Scan<size_t>(point[0]))->second << "  : " << targetAligns.find(Scan<size_t>(point[1]))->second << std::endl;
        numberOfNonTerms--;
        if(numberOfNonTerms == 0){break;}
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
