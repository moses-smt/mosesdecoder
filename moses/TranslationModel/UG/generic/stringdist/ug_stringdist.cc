#include <string>
#include <cassert>
#include <iomanip>
#include <algorithm>
#include "ug_stringdist.h"
// string distance measures
// Code by Ulrich Germann

namespace stringdist
{

  UErrorCode strip_accents(UnicodeString & trg)
  {
    UErrorCode status = U_ZERO_ERROR;
    static Transliterator *stripper
      = Transliterator::createInstance("NFD; [:M:] Remove; NFC",
				       UTRANS_FORWARD, status);
    stripper->transliterate(trg);
    return status;
  }

  char const*
  StringDiff::
  Segment::
  elabel[] = { "same", "cap", "flip", "permutation",
	       "accent", "duplication",
	       "insertion", "deletion",
	       "mismatch", "noinit" };

  StringDiff::
  StringDiff()
  {}

  StringDiff::
  StringDiff(string const& a, string const& b)
  {
    set_a(a);
    set_b(b);
    align();
  }

  StringDiff::
  Segment::
  Segment()
    : start_a(-1), end_a(-1), start_b(-1), end_b(-1), match(noinit), dist(0)
  {}

  UnicodeString const&
  StringDiff::
  set_a(string const& a)
  {
    this->a = a.c_str();
    return this->a;
  }

  UnicodeString const&
  StringDiff::
  set_b(string const& b)
  {
    this->b = b.c_str();
    return this->b;
  }

  UnicodeString const&
  StringDiff::
  get_a() const
  {
    return this->a;
  }

  UnicodeString const&
  StringDiff::
  get_b() const
  {
    return this->b;
  }

  size_t
  StringDiff::
  size()
  {
    return this->difflist.size();
  }

  // float
  // StringDiff::
  // levelshtein(bool force)
  // {
  //   align(force);
  //   float ret = 0;
  //   for (size_t i = 0; i < difflist.size(); +++i)
  //     {
  // 	Segment const& s = difflist[i];
  // 	if      (s.match == same) continue;
  // 	else if (s.match == insertion) ret += s.end_b - s.start_b;
  // 	else if (s.match == deletion)  ret += s.end_a - s.start_a;

  //     }
  // }

  void
  StringDiff::
  fillAlignmentMatrix(vector<vector<float> > & M) const
  {
    assert(a.length() && b.length());
    M.assign(a.length(),vector<float>(b.length(),0));
    int i = 0,j;
    while (i < b.length() && b[i] != a[0]) ++i;
    while (i < b.length()) M[0][i++] = 1;
    i = 0;
    while (i < a.length() && a[i] != b[0]) ++i;
    while (i < a.length()) M[i++][0] = 1;
    for (i = 1; i < a.length(); ++i)
      {
	for (j = 1; j < b.length(); ++j)
	  {
	    float & s = M[i][j];
	    s = max(M[i-1][j],M[i][j-1]);
	    if (a[i] == b[j])
	      s = max(s,M[i-1][j-1] + 1 + (a[i-1] == b[j-1] ? .1f : 0));
	  }
      }
#if 0
    string abuf,bbuf;
    a.toUTF8String(abuf);
    b.toUTF8String(bbuf);
    cout << "  " << bbuf[0];
    for (int x = 1; x < b.length(); ++x)
      cout << " " << bbuf[x];
    cout << endl;
    for (int x = 0; x < a.length(); ++x)
      {
	cout << abuf[x] << " ";
	for (int y = 0; y < b.length(); ++y)
	  cout << int(M[x][y]) << " ";
	cout << endl;
      }
#endif
  }

  float
  fillAlignmentMatrix(UChar const* a, size_t const lenA,
		      UChar const* b, size_t const lenB,
		      vector<vector<float> > & M)
  {
    M.assign(lenA,vector<float>(lenB,0));
    assert(lenA); assert(lenB);
    size_t i = 0;
    while (i < lenB && b[i] != a[0]) ++i;
    while (i < lenB) M[0][i++] = 1;
    i = 0;
    while (i < lenA && a[i] != b[0]) ++i;
    while (i < lenA) M[i++][0] = 1;
    for (i = 1; i < lenA; ++i)
      {
	for (size_t j = 1; j < lenB; ++j)
	  {
	    float & s = M[i][j];
	    s = max(M[i-1][j], M[i][j-1]);
	    if (a[i] == b[j])
	      s = max(s, M[i-1][j-1] + 1);
	  }
      }
    return M.back().back();
  }

  float
  levenshtein(UChar const* a, size_t const lenA,
	      UChar const* b, size_t const lenB)
  {
    vector<vector<float> > M;
    fillAlignmentMatrix(a,lenA,b,lenB,M);
    size_t ret = 0;
#define DEBUGME 0
#if DEBUGME
    for (size_t i = 0; i < M.size(); ++i)
      {
    	for (size_t j = 0; j < M[i].size(); ++j)
    	  cout << M[i][j] << " ";
    	cout << endl;
      }
    cout << string(25,'-') << endl;
#endif

    int i = M.size() -1;
    int j = M.back().size() -1;
    int I=i, J=j;
    for (;i >= 0 || j >= 0; --i, --j)
      {
	I=i, J=j;
	if (j>=0) while (i > 0 && M[i-1][j] == M[i][j]) --i;
	if (i>=0) while (j > 0 && M[i][j-1] == M[i][j]) --j;
	size_t ilen = I >= 0 ? I - i : 0;
	size_t jlen = J >= 0 ? J - j : 0;
	ret += max(ilen,jlen);
#if DEBUGME
	cout << I << ":" << i << " " << J << ":" << j << " " << ret << endl;
#endif
	I=i, J=j;
      }
    size_t ilen = I >= 0 ? I - i : 0;
    size_t jlen = J >= 0 ? J - j : 0;
    ret += max(ilen,jlen);
#if DEBUGME
    cout << I << ":" << i << " " << J << ":" << j << " " << ret << endl;
#endif
    return ret;
  }



  StringDiff::
  Segment::
  Segment(size_t const as, size_t const ae,
	  size_t const bs, size_t const be,
	  UnicodeString const& a,
	  UnicodeString const& b)
  {
    dist = 0;
    start_a = as; end_a = ae;
    start_b = bs; end_b = be;
    if (as == ae)
      match = bs == be ? same : insertion;
    else if (bs == be)
      match = deletion;
    else if (be-bs != ae-as)
      {
	match = mismatch;
	dist  = stringdist::levenshtein(a.getBuffer() + as, ae - as,
					b.getBuffer() + bs, be - bs);
      }
    else
      {
	match = same;
	size_t stop = ae-as;
	for (size_t i = 0; i < stop && match == same; ++i)
	  if (a[as+i] != b[bs+i]) match = mismatch;
	if (match == mismatch)
	  {
	    if (ae-as == 2 && a[as] == b[bs+1] && a[as+1] == b[bs])
	      match = flip;
	    else
	      {
		vector<UChar> x(a.getBuffer() + as, a.getBuffer() + ae);
		vector<UChar> y(b.getBuffer() + bs, b.getBuffer() + be);
		sort(x.begin(),x.end());
		sort(y.begin(),y.end());
		if (x == y) match = permutation;
		else dist = stringdist::levenshtein(a.getBuffer() + as, ae - as,
						    b.getBuffer() + bs, be - bs);
	      }
	  }
      }
    if (match == insertion)
      {
	dist = be-bs;
      }
    else if (match == deletion)
      {
	dist = ae-as;
      }
    else if (match == flip)        dist = 1;
    else if (match == permutation) dist = ae-as-1;
    if (match == mismatch)
      {
	UnicodeString ax(a,as,ae-as);
	UnicodeString bx(b,bs,be-bs);
	if (ax.toLower() == bx.toLower())
	  match = cap;
	else
	  {
	    strip_accents(ax);
	    strip_accents(bx);
	    if (ax == bx) match = accent;
	  }
      }
  }

  size_t
  StringDiff::
  align(bool force)
  {
    if (force) difflist.clear();
    if (difflist.size()) return 0;
    vector<vector<float> > M;
    fillAlignmentMatrix(M);
    // now backtrack
    int i = a.length() - 1;
    int j = b.length() - 1;
    vector<int> A(a.length(), -1);
    vector<int> B(b.length(), -1);
    while (i + j)
      {
	while (i && M[i-1][j] == M[i][j]) --i;
	while (j && M[i][j-1] == M[i][j]) --j;
	if (a[i] == b[j]) { A[i] = j; B[j] = i; }
	if (i) --i;
	if (j) --j;
      }
    i = a.length() - 1;
    j = b.length() - 1;
    vector<int> A2(a.length(), -1);
    vector<int> B2(b.length(), -1);
    while (i + j)
      {
	while (j && M[i][j-1] == M[i][j]) --j;
	while (i && M[i-1][j] == M[i][j]) --i;
	if (a[i] == b[j]) { A2[i] = j; B2[j] = i; }
	if (i) --i;
	if (j) --j;
      }
    for (size_t k = 0; k < A.size(); ++k)
      A[k] = min(A[k],A2[k]);
    for (size_t k = 0; k < B.size(); ++k)
      B[k] = min(B[k],B2[k]);

    if (a[i] == b[j]) { A[i] = j; B[j] = i; }
    i = 0;
    j = 0;
    size_t I, J;
    while (i < a.length() and j < b.length())
      {
	if (A[i] < 0)
	  {
	    I = i + 1;
	    while (I < A.size() and A[I] < 0) ++I;
	    if (i)
	      { for (J = j = A[i-1]+1; J < B.size() && B[J] < 0; ++J); }
	    else if (I < A.size())
	      { for (j = J = A[I]; j && B[j-1] < 0; --j); }
	    else J = B.size();
	    difflist.push_back(Segment(i,I,j,J,a,b));
	    i = I; j = J;
	  }
	else if (B[j] < 0)
	  {
	    for (J = j + 1; J < B.size() && B[J] < 0; ++J);
	    difflist.push_back(Segment(i,i,j,J,a,b));
	    j = J;
	  }
	else
	  {
	    I = i;
	    J = j;
	    while(I < A.size() && A[I] >= 0 && J < B.size() && B[J] >= 0)
	      { ++I; ++J; }
	    difflist.push_back(Segment(i,I,j,J,a,b));
	    i = I; j = J;
	  }
      }
    if (i < a.length() || j < b.length())
      difflist.push_back(Segment(i,a.length(),j,b.length(),a,b));

    diffcnt.assign(noinit,0);
    for (size_t i = 0; i < difflist.size(); ++i)
      {
	Segment & s = difflist[i];
	if (s.match == insertion and
	    ((s.start_a and a[s.start_a - 1] == b[s.start_b]) or
	     (s.end_a < a.length() and a[s.end_a] == b[s.start_b])))
	  {
	    bool sameletter = true;
	    for (int i = s.start_b + 1; sameletter and i < s.end_b; ++i)
	      sameletter = b[i] == b[i-1];
	    if (sameletter) s.match = duplication;
	  }
	else if (s.match == deletion and
		 ((s.start_b and b[s.start_b - 1] == a[s.start_a]) or
		  (s.end_b < b.length() and b[s.end_b] == a[s.start_a])))
	  {
	    bool sameletter = true;
	    for (int i = s.start_a + 1; sameletter and i < s.end_a; ++i)
	      sameletter = a[i] == a[i-1];
	    if (sameletter) s.match= duplication;
	  }
	++diffcnt[s.match];
      }
    return 0;
  }

  void
  StringDiff::
  showDiff(std::ostream& out)
  {
    if (difflist.size() == 0) align();
    vector<size_t> fromEnd(difflist.size(),0);
    for (int d = difflist.size()-1; d-- > 0;)
      {
	fromEnd[d] = a.length() - difflist[d].end_a;
	// cout << d << " " << fromEnd[d] << " "
	//      << difflist[d].start_a << "-"
	//      << difflist[d].end_a << endl;
      }
    for (size_t d = 0; d < difflist.size(); ++d)
      {
	Segment const& s = difflist[d];
	UnicodeString aseg,bseg;
	a.extract(s.start_a, s.end_a - s.start_a, aseg);
	b.extract(s.start_b, s.end_b - s.start_b, bseg);
	string abuf,bbuf;
	aseg.toUTF8String(abuf);
	bseg.toUTF8String(bbuf);
	out << abuf << " ";
	out << bbuf << " ";
	out << s.label() << " "
	    << s.dist << " "
	    << fromEnd[d]
	    << endl;
      }
  }

  char const*
  StringDiff::
  Segment::
  label() const
  {
    return elabel[this->match];
  }

  StringDiff::Segment const&
  StringDiff::
  operator[](uint32_t const i) const
  {
    return difflist.at(i);
  }

  vector<int> const&
  StringDiff::
  getFeatures() const
  {
    return diffcnt;
  }

}
