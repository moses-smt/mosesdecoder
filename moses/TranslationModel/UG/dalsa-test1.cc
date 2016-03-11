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
  for (int i = 1; i < argc;)
    {
      fname1.push_back(argv[i]);
      fname2.push_back(argv[i+1]);
      boost::iostreams::filtering_istream en, it;
      
      ugdiss::open_input_stream(argv[i++], en);
      ugdiss::open_input_stream(argv[i++], it);
      string eline,iline;
      vector<string> doc1,doc2;
      while (getline(en, eline)) doc1.push_back(eline);
      while (getline(it, iline)) doc2.push_back(iline);
      d1.push_back(vector<float>(LSA1.cols()));
      d2.push_back(vector<float>(LSA2.cols()));
      LSA1.adapt(doc1, &d1.back()[0]);
      LSA2.adapt(doc2, &d2.back()[0]);
      // cout << fname1.back() << " " << dot(d1.back(), d2.back()) << endl;
   }

  for (size_t i = 0; i < d1.size(); ++i)
    for (size_t k = i; k < d2.size(); ++k)
      cout << dot(d1[i],d2[k]) << " " << fname1[i] << " " << fname2[k] << endl;
}
