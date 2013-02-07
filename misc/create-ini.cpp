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
  string implementation;
  vector<string> toks;
  FF(const string &line)
  {
    vector<string> toks = Tokenize(line, ":");
  }
};

class LM : public FF
{
public:
  string otherArgs, path;
  int order, factor;

  LM(const string &line)
  :FF(line)
  {
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

class Pt : public FF
{
  string path;
  int numFeatures;

  Pt(const string &line)
  :FF(line)
  {
    path = toks[0];
    numFeatures = 5;    
  }
};

string iniPath;
vector<string> ptVec, reorderingVec;
vector<LM> lmVec;

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

  for (size_t i = 0; i < lmVec.size(); ++i) {
    const LM &lm = lmVec[i];
    strme << lm.implementation << i << " "
          << " order=" << lm.order 
          << " factor=" << lm.factor
          << " path=" << lm.path
          << " " << lm.otherArgs
          << endl;

    weightStrme << << lm.implementation << i << "= 0.5" << endl;
  }

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
      ptVec.push_back(path);
    }
    else if (key == "-reordering-table") {
      ++i;
      string path = argv[i];
      reorderingVec.push_back(path);
    }
    else if (key == "-lm") {
      ++i;
      string line = argv[i];
      LM lm(line);
      lmVec.push_back(lm);
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




