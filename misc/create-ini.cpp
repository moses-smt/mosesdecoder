#include <typeinfo>
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
  friend std::ostream& operator<<(std::ostream &out, const FF&);

  virtual void Output(std::ostream &out) const = 0;
public:
  vector<string> toks;
  string name;
  string path;
  int index, numFeatures;
  
  FF(const string &line)
  {
    toks = Tokenize(line, ":");
  }

  virtual string ffType() const= 0;
};

std::ostream& operator<<(std::ostream &out, const FF &model)
{
  model.Output(out);
  return out;
}

class LM : public FF
{
  static int s_index;

  void Output(std::ostream &out) const
  {
    out << name << index << " "
        << " order=" << order 
        << " factor=" << factor
        << " path=" << path
        << " " << otherArgs;
  }
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
      case 0: name = "SRILM"; break;
      case 1: name = "IRSTLM"; break;
      case 8: name = "KENLM"; otherArgs = "lazyken=0"; break;
      case 9: name = "KENLM"; otherArgs = "lazyken=1"; break;
    }
  }

  string ffType() const
  { return "LM"; }  
};

class RO : public FF
{
  static int s_index;

  void Output(std::ostream &out) const
  {
    out << name << index << " "
        << " path=" << path;
  }
public:

  RO(const string &line)
  :FF(line)
  {
    name = "LexicalReordering";
    numFeatures = 6;
    path = toks[0];
  }

  string ffType() const
  { return "RO"; } 
};

class Pt : public FF
{
  static int s_index;

  void Output(std::ostream &out) const
  {
    out << name << index << " "
        << " path=" << path;
  }
public:
  int numFeatures;
  vector<int> inFactor, outFactor;

  Pt(const string &line)
  :FF(line)
  {
    index = s_index++;
    name = "PhraseModel";
    numFeatures = 5;    
    path = toks[0];

    inFactor.push_back(0);
    outFactor.push_back(0);
  }

  string ffType() const
  { return "Pt"; } 
};

int Pt::s_index = 0;


string iniPath;
vector<FF*> ffVec;

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
    const FF &ff = *ffVec[i];

    strme << ff << endl;
  
    OutputWeights(weightStrme, ff);
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
      Pt *model = new Pt(argv[i]);
      ffVec.push_back(model);
    }
    else if (key == "-reordering-table") {
      ++i;
      RO *model = new RO(argv[i]);
      ffVec.push_back(model);
    }
    else if (key == "-lm") {
      ++i;
      LM *model = new LM(argv[i]);
      ffVec.push_back(model);
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




