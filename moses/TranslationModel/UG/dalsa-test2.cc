// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "moses/FF/LSA.h"
#include "generic/file_io/ug_stream.h"
#include <iostream>

using namespace Moses;
using namespace std;

LsaModel LSA1,LSA2;

float 
dot(vector<float> const& a, vector<float> const& b)
{
  float ret = 0;
  for (size_t i = 0; i < a.size(); ++i) ret += a[i] * b[i];
}

int main(int argc, char* argv[])
{
  LSA1.open("mdl/","en","it");
  LSA2.open("mdl/","it","en");
  vector<string> fname1,fname2;
  vector<vector<float> > d1, d2;

  boost::iostreams::filtering_istream doc_in;
      
  ugdiss::open_input_stream(argv[1], doc_in);
  string line;
  vector<string> doc;
  while (getline(doc_in, line)) doc.push_back(line);
  LsaTermMatcher match(&LSA1, doc);
  for (int i = 2; i < argc; ++i)
    cout << exp(match(LSA1.V2[argv[i]])) << " " << argv[i] << endl;
    

  // for (size_t id = 0; LSA1.tvec2(id); ++id)
}
