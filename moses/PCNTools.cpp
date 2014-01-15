#include "PCNTools.h"

#include <iostream>
#include <cstdlib>
#include "Util.h"
#include "util/exception.hh"

using namespace std;

namespace PCN
{

const std::string chars = "'\\";
const char& quote = chars[0];
const char& slash = chars[1];

// safe get
inline char get(const std::string& in, int c)
{
  if (c < 0 || c >= (int)in.size()) return 0;
  else return in[(size_t)c];
}

// consume whitespace
inline void eatws(const std::string& in, int& c)
{
  while (get(in,c) == ' ') {
    c++;
  }
}

std::string getString(const std::string& in, int &c)
{
  std::string ret;
  eatws(in,c);
  while (c < (int)in.size() && get(in,c) != ' ' && get(in,c) != ')' && get(in,c) != ',') {
    ret += get(in,c++);
  }
  eatws(in,c);
  return ret;
}

// from 'foo' return foo
std::string getEscapedString(const std::string& in, int &c)
{
  eatws(in,c);
  if (get(in,c++) != quote) return "ERROR";
  std::string res;
  char cur = 0;
  do {
    cur = get(in,c++);
    if (cur == slash) {
      res += get(in,c++);
    } else if (cur != quote) {
      res += cur;
    }
  } while (get(in,c) != quote && (c < (int)in.size()));
  c++;
  eatws(in,c);
  return res;
}

// basically atof
float getFloat(const std::string& in, int &c)
{
  std::string tmp;
  eatws(in,c);
  while (c < (int)in.size() && get(in,c) != ' ' && get(in,c) != ')' && get(in,c) != ',') {
    tmp += get(in,c++);
  }
  eatws(in,c);
  return atof(tmp.c_str());
}

// basically atof
int getInt(const std::string& in, int &c)
{
  std::string tmp;
  eatws(in,c);
  while (c < (int)in.size() && get(in,c) != ' ' && get(in,c) != ')' && get(in,c) != ',') {
    tmp += get(in,c++);
  }
  eatws(in,c);
  return atoi(tmp.c_str());
}

// parse ('foo', 0.23)
CNAlt getCNAlt(const std::string& in, int &c)
{
  if (get(in,c++) != '(') {
    std::cerr << "PCN/PLF parse error: expected ( at start of cn alt block\n";  // throw "expected (";
    return CNAlt();
  }
  std::string word = getEscapedString(in,c);
  if (get(in,c++) != ',') {
    std::cerr << "PCN/PLF parse error: expected , after string\n";  // throw "expected , after string";
    return CNAlt();
  }
  size_t cnNext = 1;

  // read all tokens after the 1st
  std::vector<string> toks;
  toks.push_back(getString(in,c));
  while (get(in,c) == ',') {
    c++;
    string tok = getString(in,c);
    toks.push_back(tok);
  }

  std::vector<float> probs;

  // dense scores
  size_t ind;
  for (ind = 0; ind < toks.size() - 1; ++ind) {
    const string &tok = toks[ind];

    if (tok.find('=') == tok.npos) {
      float val = Moses::Scan<float>(tok);
      probs.push_back(val);
    } else {
      // beginning of sparse feature
      break;
    }
  }

  // sparse features
  std::map<string, float> sparseFeatures;
  for (; ind < toks.size() - 1; ++ind) {
    const string &tok = toks[ind];
    vector<string> keyValue = Moses::Tokenize(tok, "=");
    UTIL_THROW_IF2(keyValue.size() != 2, "Format error: " << tok);
    float prob = Moses::Scan<float>(keyValue[1]);
    sparseFeatures[ keyValue[0] ] = prob;
  }

  //last item is column increment
  cnNext = Moses::Scan<size_t>(toks.back());

  if (get(in,c++) != ')') {
    std::cerr << "PCN/PLF parse error: expected ) at end of cn alt block\n";  // throw "expected )";
    return CNAlt();
  }
  eatws(in,c);
  return CNAlt(word, probs, sparseFeatures, cnNext);
}

// parse (('foo', 0.23), ('bar', 0.77))
CNCol getCNCol(const std::string& in, int &c)
{
  CNCol res;
  if (get(in,c++) != '(') return res;  // error
  eatws(in,c);
  while (1) {
    if (c > (int)in.size()) {
      break;
    }
    if (get(in,c) == ')') {
      c++;
      eatws(in,c);
      break;
    }
    if (get(in,c) == ',' && get(in,c+1) == ')') {
      c+=2;
      eatws(in,c);
      break;
    }
    if (get(in,c) == ',') {
      c++;
      eatws(in,c);
    }
    res.push_back(getCNAlt(in, c));
  }
  return res;
}

// parse ((('foo', 0.23), ('bar', 0.77)), (('a', 0.3), ('c', 0.7)))
CN parsePCN(const std::string& in)
{
  CN res;
  int c = 0;
  if (in[c++] != '(') return res; // error
  while (1) {
    if (c > (int)in.size()) {
      break;
    }
    if (get(in,c) == ')') {
      c++;
      eatws(in,c);
      break;
    }
    if (get(in,c) == ',' && get(in,c+1) == ')') {
      c+=2;
      eatws(in,c);
      break;
    }
    if (get(in,c) == ',') {
      c++;
      eatws(in,c);
    }
    res.push_back(getCNCol(in, c));
  }
  return res;
}


}

