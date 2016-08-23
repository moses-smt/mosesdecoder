// -*- c++ -*-
// Parallel text alignment via IBM1 / raw counts of word alignments
// aiming at high precision (to seed Yawat alignments)
// This program is tailored for use with Yawat.
// Written by Ulrich Germann.

#include <string>

#include <unicode/stringpiece.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/utf8.h>

#include "moses/TranslationModel/UG/generic/file_io/ug_stream.h"
#include "moses/TranslationModel/UG/mm/tpt_tokenindex.h"
#include <boost/unordered_map.hpp>
#include "moses/TranslationModel/UG/mm/tpt_pickler.h"
#include "moses/TranslationModel/UG/mm/ug_mm_2d_table.h"

using namespace std;
using namespace ugdiss;

typedef mm2dTable<id_type,id_type,uint32_t,uint32_t> table_t;

class IBM1
{
public:
  table_t COOC;
  TokenIndex V1,V2;

  void
  align(string const& s1, string const& s2, vector<int>& aln) const;

  void
  align(vector<id_type> const& x1,
	vector<id_type> const& x2,
	vector<int>& aln) const;

  void
  fill_amatrix(vector<id_type> const& x1,
	       vector<id_type> const& x2,
	       vector<vector<int> >& aln) const;

  void
  open(string const base, string const L1, string const L2);
};

void
IBM1::
open(string const base, string const L1, string const L2)
{
  V1.open(base+L1+".tdx");
  V2.open(base+L2+".tdx");
  COOC.open(base+L1+"-"+L2+".lex");
}

void
IBM1::
align(string const& s1, string const& s2, vector<int>& aln) const
{
  vector<id_type> x1,x2;
  V1.fillIdSeq(s1,x1);
  V2.fillIdSeq(s2,x2);
  align(x1,x2,aln);
}

 static UnicodeString apos   = UnicodeString::fromUTF8(StringPiece("'"));

string
u(StringPiece str, size_t start, size_t stop)
{
  string ret;
  UnicodeString::fromUTF8(str).tempSubString(start,stop).toUTF8String(ret);
  return ret;
}

void
IBM1::
fill_amatrix(vector<id_type> const& x1,
	     vector<id_type> const& x2,
	     vector<vector<int> >& aln) const
{
  aln.assign(x1.size(),vector<int>(x2.size()));
  for (size_t i = 0; i < x1.size(); ++i)
    for (size_t k = 0; k < x2.size(); ++k)
      aln[i][k] = COOC[x1[i]][x2[k]];
#if 0
  cout << setw(10) << " ";
  for (size_t k = 0; k < x2.size(); ++k)
    cout << setw(7) << right << u(V2[x2[k]],0,6);
  cout << endl;
  for (size_t i = 0; i < x1.size(); ++i)
    {
      cout << setw(10) << u(V1[x1[i]],0,10);
      for (size_t k = 0; k < x2.size(); ++k)
	{
	  if (aln[i][k] > 999999)
	    cout << setw(7) << aln[i][k]/1000 << " K";
	  else
	    cout << setw(7) << aln[i][k];
	}
      cout << endl;
    }
#endif
}


void
IBM1::
align(vector<id_type> const& x1,
      vector<id_type> const& x2,
      vector<int>& aln) const
{
  vector<vector<int> > M;
  // fill_amatrix(x1,x2,M);
  vector<int> i1(x1.size(),0), max1(x1.size(),0);
  vector<int> i2(x2.size(),0), max2(x2.size(),0);
  aln.clear();
  for (size_t i = 0; i < i1.size(); ++i)
    {
      for (size_t k = 0; k < i2.size(); ++k)
	{
	  int c = COOC[x1[i]][x2[k]];
	  if (c >  max1[i]) { i1[i] = k; max1[i] = c; }
	  if (c >= max2[k]) { i2[k] = i; max2[k] = c; }
	}
    }
  for (size_t i = 0; i < i1.size(); ++i)
    {
      if (max1[i] && i2[i1[i]] == i)
	{
	  aln.push_back(i);
	  aln.push_back(i1[i]);
	}
    }
}

int main(int argc, char* argv[])
{
  IBM1 ibm1;
  ibm1.open(argv[1],argv[2],argv[3]);
  string line1,line2,sid;
  while (getline(cin,sid))
    {
      if (!getline(cin,line1)) assert(false);
      if (!getline(cin,line2)) assert(false);
      vector<int> a;
      vector<id_type> s1,s2;
      ibm1.V1.fillIdSeq(line1,s1);
      ibm1.V2.fillIdSeq(line2,s2);
      ibm1.align(s1,s2,a);
      cout << sid;
      for (size_t i = 0; i < a.size(); i += 2)
	cout << " " << a[i] << ":" << a[i+1] << ":unspec";
      cout << endl;
      // cout << line1 << endl;
      // cout << line2 << endl;
      // for (size_t i = 0; i < a.size(); i += 2)
      // 	cout << ibm1.V1[s1[a[i]]] << " - "
      // 	     << ibm1.V2[s2[a[i+1]]] << endl;
    }
  // cout << endl;
}
