#include <fstream>
#include <iostream>
#include<string>
#include<sstream>
#include<vector>
#include<map>
#include "Desegmenter.h"
#include <boost/algorithm/string/replace.hpp>

using namespace std;

namespace Moses
{
void Desegmenter::Load(const string filename)
{

  std::ifstream myFile(filename.c_str() );
  if (myFile.is_open()) {
    cerr << "Desegmentation File open successful." << endl;
    string line;
    while (getline(myFile, line)) {
      stringstream ss(line);
      string token;
      vector<string> myline;
      while (getline(ss, token, '\t')) {
        myline.push_back(token);
      }
      mmDesegTable.insert(pair<string, string>(myline[2], myline[1] ));
    }
    myFile.close();
  } else
    cerr << "open() failed: check if Desegmentation file is in right folder" << endl;
}


vector<string> Desegmenter::Search(string myKey)
{
  multimap<string, string>::const_iterator  mmiPairFound = mmDesegTable.find(myKey);
  vector<string> result;
  if (mmiPairFound != mmDesegTable.end()) {
    size_t nNumPairsInMap = mmDesegTable.count(myKey);
    for (size_t nValuesCounter = 0; nValuesCounter < nNumPairsInMap; ++nValuesCounter) {
      if (mmiPairFound != mmDesegTable.end())	{
        result.push_back(mmiPairFound->second);
      }
      ++mmiPairFound;
    }
    return result;
  } else {
    string rule_deseg ;
    rule_deseg = ApplyRules(myKey);
    result.push_back(rule_deseg);
    return result;
  }
}


string Desegmenter::ApplyRules(string & segToken)
{

  string desegToken=segToken;
  if (!simple) {
    boost::replace_all(desegToken, "l+ All", "ll");
    boost::replace_all(desegToken, "l+ Al", "ll");
    boost::replace_all(desegToken, "y+ y ", "y");
    boost::replace_all(desegToken, "p+ ", "t");
    boost::replace_all(desegToken, "' +", "}");
    boost::replace_all(desegToken, "y +", "A");
    boost::replace_all(desegToken, "n +n", "n");
    boost::replace_all(desegToken, "mn +m", "mm");
    boost::replace_all(desegToken, "En +m", "Em");
    boost::replace_all(desegToken, "An +lA", "Em");
    boost::replace_all(desegToken, "-LRB-", "(");
    boost::replace_all(desegToken, "-RRB-", ")");
  }

  boost::replace_all(desegToken, "+ +", "");
  boost::replace_all(desegToken, "+ ", "");
  boost::replace_all(desegToken, " +", "");

  return desegToken;
}

Desegmenter::~Desegmenter()
{}

}
