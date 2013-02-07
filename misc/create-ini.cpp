#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>

using namespace std;

std::vector<std::string> Tokenize(const std::string& str,
    const std::string& delimiters = " \t");

template<typename T>
inline T Scan(const std::string &input)
{
  std::stringstream stream(input);
  T ret;
  stream >> ret;
  return ret;
}

class FF
{
public:
  vector<string> toks;
  string implementation;
  string path;
  int numFeatures;
  
  FF(const string &line)
  {
    toks = Tokenize(line, ":");
  }
};

class LM : public FF
{
public:
  string otherArgs;
  int order, factor;

  LM(const string &line)
  :FF(line)
  {
    numFeatures = 1;

    factor = Scan<int>(toks[0]);
    order = Scan<int>(toks[1]);
    path = toks[2];
    int implNum = Scan<int>(toks[3]);

    switch (implNum)
    {
      case 0: implementation = "SRILM"; break;
      case 1: implementation = "IRSTLM"; break;
      case 8: implementation = "KENLM"; otherArgs = "lazyken=0"; break;
      case 9: implementation = "KENLM"; otherArgs = "lazyken=1"; break;
    }
  }
};

class RO : public FF
{
  RO(const string &line)
  :FF(line)
  {
    implementation = "LexicalReordering";
    numFeatures = 6;
    path = toks[0];
  }
};

class Pt : public FF
{
  int numFeatures;

  Pt(const string &line)
  :FF(line)
  {
    implementation = "PhraseModel";
    numFeatures = 5;    
    path = toks[0];
  }
};

string iniPath;
vector<FF> ffVec;

void OutputWeights(stringstream &weightStrme, const FF &ff)
{
}

void Output()
{
  ofstream strme(iniPath.c_str());
  stringstream weightStrme;

  weightStrme << "[weight]" << endl;

  strme << "[input-factors]" << endl;
  strme << "0" << endl;

  strme << "[mapping]" << endl;
  strme << "0 T 0" << endl;

  strme << "[distortion-limit]" << endl;
  strme << "6" << endl;

  strme << "[feature]" << endl;
  for (size_t i = 0; i < ffVec.size(); ++i) {
    const FF &ff = ffVec[i];

    const LM *lm = dynamic_cast<const LM*>(&ff);
    if (lm) {
      strme << lm->implementation << i << " "
            << " order=" << lm->order 
            << " factor=" << lm->factor
            << " path=" << lm->path
            << " " << lm->otherArgs
            << endl;
    }
    OutputWeights(weightStrme, ff);
  }

/*
  for (size_t i = 0; i < reorderingVec.size(); ++i) {
    const string &path = reorderingVec[i];
    strme << "LexicalReordering" << i << " "
          << "path=" << path 
          << endl;

    weightStrme << << lm.implementation << i << "= 0.5" << endl;
  }

  for (size_t i = 0; i < ptVec.size(); ++i) {
    const string &path = ptVec[i];
    strme << "PhraseModel" << i << " "
          << "path=" << path 
          << endl;
  }
*/

  strme << weightStrme.str();

  strme.close();
}

int main(int argc, char **argv)
{
  for (int i = 0; i < argc; ++i) {
    string key(argv[i]);
    
    if (key == "-phrase-translation-table") {
      ++i;
      string path = argv[i];
      ffVec.push_back(path);
    }
    else if (key == "-reordering-table") {
      ++i;
      string path = argv[i];
      ffVec.push_back(path);
    }
    else if (key == "-lm") {
      ++i;
      string line = argv[i];
      LM lm(line);
      ffVec.push_back(lm);
    }
    else if (key == "-config") {
      ++i;
      iniPath = argv[i];
    }
  }

  Output();
}



std::vector<std::string> Tokenize(const std::string& str,
    const std::string& delimiters)
{
  std::vector<std::string> tokens;
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }

  return tokens;
}




