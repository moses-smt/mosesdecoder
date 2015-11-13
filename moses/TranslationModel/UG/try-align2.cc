#include "mm/ug_bitext.h"
#include <boost/format.hpp>
// #include <unicode/stringpiece.h>
#include <unicode/translit.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/utf8.h>
#include "moses/TranslationModel/UG/generic/stringdist/ug_stringdist.h"

using namespace std;
using namespace Moses;
using namespace ugdiss;
using namespace Moses::bitext;

typedef L2R_Token<SimpleWordId> Token;
typedef mmTtrack<Token> ttrack_t;
typedef mmTSA<Token>     tsa_t;
typedef vector<Moses::bitext::PhrasePair<Token> > pplist_t;
typedef pair<ushort,ushort> span_t;

TokenIndex V1,V2;
boost::shared_ptr<ttrack_t> T1,T2;
tsa_t I1,I2;
mmBitext<Token> BT;

float lbop_level = .05;
#define smooth 1
namespace stats
{
  using namespace Moses::bitext;
  float
  pmi(size_t j,size_t m1, size_t m2, size_t N)
  {
#if smooth
    float p1 = lbop(N,m1,lbop_level);
    float p2 = lbop(N,m2,lbop_level);
    float p12 = lbop(N,j,lbop_level);
    return log(p12) - log(p1) - log(p2);
#else
    return log(j) + log(N) - log(m1) - log(m2);
#endif
  }

  float
  npmi(size_t j,size_t m1, size_t m2, size_t N)
  {
#if smooth
    // cout << j << " " << m1 << " " << m2 << " " << N << endl;
    float p1 = lbop(N,m1,lbop_level);
    float p2 = lbop(N,m2,lbop_level);
    float p12 = lbop(N,j,lbop_level);
    return (log(p12) - log(p1) - log(p2)) / -log(p12);
#else
    return pmi(j,m1,m2,N) / (log(N) - log(j));
#endif
  }

  float
  mi(size_t j,size_t m1, size_t m2, size_t N)
  {
    float ret = 0;
    if (j)    ret += float(j)/N * pmi(j,m1,m2,N);
    if (m1>j) ret += float(m1-j)/N * pmi(m1-j,m1,N-m2,N);
    if (m2>j) ret += float(m2-j)/N * pmi(m2-j,N-m1,m2,N);
    if (N>m1+m2-j) ret += float(N-m1-m2+j)/N * pmi(N-m1-m2+j,N-m1,N-m2,N);
    return ret;
  }
}

struct SinglePhrase
{
  typedef map<uint64_t,SPTR<SinglePhrase> > cache_t;
  uint64_t pid; // phrase id
  vector<ttrack::Position> occs; // occurrences
};


struct PhrasePair2
{
  struct score_t;
  uint64_t p1,p2;
  ushort s1,e1,s2,e2;
  int parent;

  struct stats_t
  {
    typedef map<pair<uint64_t,uint64_t>, SPTR<stats_t> > cache_t;
    size_t m1,m2,j;
    float  npmi; // normalized point-wise mutual information
    float   pmi; // point-wise mutual information
    float    mi; // mutual information
    float score;

    void
    set(vector<ttrack::Position> const& o1,
	vector<ttrack::Position> const& o2,
	size_t const N)
    {
      m1 = m2 = j = 0;
      size_t i1=0,i2=0;
      while (i1 < o1.size() && i2 < o2.size())
	{
	  if (i1 && o1[i1].sid == o1[i1-1].sid) { ++i1; continue; }
	  if (i2 && o2[i2].sid == o2[i2-1].sid) { ++i2; continue; }

	  if (o1[i1].sid == o2[i2].sid) { ++j; ++i1; ++i2; ++m1; ++m2; }
	  else if (o1[i1].sid < o2[i2].sid) { ++i1; ++m1; }
	  else { ++i2; ++m2; }
	}
      // for (++i1; i1 < o1.size(); ++i1)
      // 	if (o1[i1-1].sid != o1[i1].sid) ++m1;
      // for (++i2; i2 < o2.size(); ++i2)
      // 	if (o2[i2-1].sid != o2[i2].sid) ++m2;

      m1 = 1; m2 = 1;
      for (i1=1; i1 < o1.size(); ++i1)
       	if (o1[i1-1].sid != o1[i1].sid) ++m1;
      for (i2=1; i2 < o2.size(); ++i2)
	if (o2[i2-1].sid != o2[i2].sid) ++m2;

      this->mi = stats::mi(j,m1,m2,N);
      this->pmi = stats::pmi(j,m1,m2,N);
      this->npmi = stats::npmi(j,m1,m2,N);
      // float z = float(m1)/N * float(m2)/N;
      // float hmean = 2.*j/(m1+m2);
      this->score = npmi; // npmi; // hmean; // /sqrt(z);
    }
  } stats;

  PhrasePair2(ushort s1_=0, ushort e1_=0, ushort s2_=0, ushort e2_=0)
    : s1(s1_), e1(e1_), s2(s2_), e2(e2_), parent(-1) { }


  bool
  operator<(PhrasePair2 const& other) const
  {
    return (this->stats.score == other.stats.score
	    ? (e1-s1 + e2-s2 > other.e1-other.s1 + other.e2-other.s2)
	    : (this->stats.score > other.stats.score));
  }

  size_t len1() const { return e1 - s1; }
  size_t len2() const { return e2 - s2; }
  bool includes(PhrasePair2 const& o) const
  {
    return s1 <= o.s1 && e1 >= o.e1 && s2 <= o.s2 && e2 >= o.e2;
  }

};

SinglePhrase::cache_t cache1,cache2;
PhrasePair2::stats_t::cache_t ppcache;


struct SortByPositionInCorpus
{
  bool
  operator()(ttrack::Position const& a,
	     ttrack::Position const& b) const
  {
    return a.sid != b.sid ? a.sid < b.sid : a.offset < b.offset;
  }
};


void
getoccs(tsa_t::tree_iterator const& m,
	vector<ttrack::Position>& occs)
{
  occs.clear();
  occs.reserve(m.approxOccurrenceCount()+10);
  tsa::ArrayEntry I(m.lower_bound(-1));
  char const* stop = m.upper_bound(-1);
  do {
    m.root->readEntry(I.next,I);
    occs.push_back(I);
  } while (I.next != stop);
  sort(occs.begin(),occs.end(),SortByPositionInCorpus());
}

void
lookup_phrases(vector<id_type> const& snt,
	       TokenIndex& V, ttrack_t const& T,
	       tsa_t const& I, SinglePhrase::cache_t& cache,
	       vector<vector<SPTR<SinglePhrase> > >& dest)
{
  dest.resize(snt.size());
  for (size_t i = 0; i < snt.size(); ++i)
    {
      tsa_t::tree_iterator m(&I);
      dest[i].clear();
      for (size_t k = i; k < snt.size() && m.extend(snt[k]); ++k)
	{
	  if (m.approxOccurrenceCount() < 3) break;
	  // if (k - i > 0) break;
	  SPTR<SinglePhrase>& o = cache[m.getPid()];
	  if (!o)
	    {
	      o.reset(new SinglePhrase());
	      o->pid = m.getPid();
	      getoccs(m,o->occs);
	    }
	  dest[i].push_back(o);
	}
    }
}


struct
RowIndexSorter
{
  vector<vector<float> > const& M;
  size_t const my_col;
  RowIndexSorter(vector<vector<float> > const& m, size_t const c)
    : M(m), my_col(c) { }

  template<typename T>
  bool
  operator()(T const& a, T const& b) const
  {
    return M.at(a).at(my_col) > M.at(b).at(my_col);
  }
};

struct
ColIndexSorter
{
  vector<vector<float> > const& M;
  size_t const my_row;
  ColIndexSorter(vector<vector<float> > const& m, size_t const r)
    : M(m), my_row(r) { }

  template<typename T>
  bool
  operator()(T const& a, T const& b) const
  {
    return M.at(my_row).at(a) > M[my_row].at(b);
  }

};

template<typename Token>
class
npmi_scorer1 : public Moses::bitext::PhrasePair<Token>::Scorer
{
public:
  float operator()(PhrasePair<Token>& pp) const
  {
#if 0
    cout << pp.raw1 << " " << pp.sample1 << " " << pp.good1 << " "
	 << pp.raw2 << " " << pp.sample2 << " " << pp.good2 << " "
	 << pp.joint << " " << __FILE__ << ":" << __LINE__ << endl;
#endif
    pp.good2 = ceil(pp.raw2 * float(pp.good1)/pp.raw1);
    size_t N  = ceil(BT.T1->numTokens() * float(pp.good1)/pp.raw1);
    return pp.score = stats::npmi(pp.joint,pp.good1,pp.good2,N);
  }
};


class Alnhyp
{
  ushort s1,s2,e1,e2;
  float score;
};


size_t
lcs(string const a, string const b)
{
  using namespace stringdist;
  if (a == b) return a.size();
  StringDiff diff(a,b);
  size_t ret = 0;
  size_t len = 0;
  // size_t th = min(size_t(4),min(a.size(),b.size()));
  for (size_t i = 0; i < diff.size(); ++i)
    {
      StringDiff::Segment const& s = diff[i];
      if (s.match != StringDiff::same && s.match != StringDiff::cap)
	{
	  if (len > ret) ret = len;
	  len = 0;
	  continue;
	}
      len += s.end_a - s.start_a;
    }
  if (len > ret) ret = len;
  return ret;
}

size_t
mapstring(string const& utf8,
	  UnicodeString& U,
	  vector<int>& c2w,
	  vector<int>* wlen=NULL)
{
  static UChar space = UnicodeString(" ")[0];
  assert(utf8.at(0) != ' ');
  U = UnicodeString(utf8.c_str()).toLower();
  stringdist::strip_accents(U);
  c2w.assign(U.length(),-1);
  size_t k = 0;
  size_t z = 0;
  for (int i = 0; i < U.length(); ++i)
    if (U[i] == space) { if (wlen) wlen->push_back(i-z); z = ++k; }
    else c2w[i] = k;
  assert(c2w.back() >= 0);
  if (wlen) wlen->push_back(U.length()-z);
  return k+1;
}

void
align_letters(UnicodeString const& A, vector<int> const& a2p,
	      UnicodeString const& B, vector<int> const& b2p,
	      vector<vector<int> >& W)
{
  vector<vector<int> > M(A.length(),vector<int>(B.length(),0));
  for (int a = 0; a < A.length(); ++a)
    {
      for (int b = 0; b < B.length(); ++b)
	{
	  if (A[a] != B[b] || a2p[a] < 0 || b2p[b] < 0)
	    continue;
	  M[a][b] = (a && b) ? M[a-1][b-1] + 1 : 1;
	  int& x = W[a2p[a]][b2p[b]];
	  x = max(x,M[a][b]);
	}
    }
  // string astring; A.toUTF8String(astring);
  // string bstring; B.toUTF8String(bstring);
  // cout << astring << "\n" << bstring << endl;
  // for (size_t r = 0; r < W.size(); ++r)
  //   {
  //     BOOST_FOREACH(int x, W[r]) cout << setw(3) << x;
  //     cout << endl;
  //   }
}

void
map_back(vector<vector<int> > const& W,
	 vector<vector<int> > & X,
	 vector<uchar> const & aln)
{
  for (size_t i = 0; i < aln.size(); i += 2)
    {
      vector<int> const& w = W.at(aln[i+1]);
      vector<int>& x = X.at(aln[i]);
      assert(x.size() == w.size());
      for (size_t k = 0; k < x.size(); ++k)
	x[k] = max(w[k],x[k]);
    }
}


void trymatch3(vector<PhrasePair<Token> > const& tcands,
	       UnicodeString const& T, size_t const tlen,
	       vector<int> const& t2p,
	       TokenIndex const& V2, vector<vector<int> >&X)
{
  BOOST_FOREACH(PhrasePair<Token> const& pp, tcands)
    {
      UnicodeString H; vector<int> h2p;
      string hstring = toString(V2, pp.start2, pp.len2);
      size_t hlen = mapstring(hstring,H,h2p);
      vector<vector<int> > W(hlen,vector<int>(tlen,0));
      align_letters(H, h2p, T, t2p, W);
#if 0
      string s; S.toUTF8String(s);
      string h; H.toUTF8String(h);
      string t; T.toUTF8String(t);
      cout << s << endl << h << endl << t << endl;
      cout << slen << " " << tlen << endl;
      cout << "W: " << W.size() << " rows; " << W[0].size() << " cols" << endl;
      cout << "X: " << X.size() << " rows; " << X[0].size() << " cols" << endl;
      cout << "aln: ";
      for (size_t a = 0; a < pp.aln.size(); a +=2)
	cout  << int(pp.aln[a]) << "-" << int(pp.aln[a+1]) << " ";
      cout << endl;
#endif
      map_back(W,X,pp.aln);
    }
}

void minmatch_filter(vector<vector<int> > & X,
		     vector<int> const& len1,
		     vector<int> const& len2)
{
  // compute marginals
  vector<int> m1(X.size(),0), m2(X.at(0).size(),0);
  for (size_t r = 0;  r < X.size(); ++r)
    for (size_t c = 0; c < X[r].size(); ++c)
      {
	if (X[r][c] == 0) continue;
	m1[r] += X[r][c];
	m2[c] += X[r][c];
      }

  bool sth_changed = true;
  while (sth_changed)
    {
      sth_changed = false;
      for (size_t r = 0;  r < m1.size(); ++r)
	{
	  if (m1[r] && m1[r] < max(2,min(5,len1[r]/2)))
	    {
	      sth_changed = true;
	      for (size_t c = 0; c < X[r].size(); ++c)
		{
		  m2[c] -= X[r][c];
		  X[r][c] = 0;
		}
	      m1[r] = 0;
	    }
	}

      for (size_t c = 0; c < m2.size(); ++c)
	{
	  if (m2[c] && m2[c] < max(2,min(5,len2[c]/2)))
	    {
	      sth_changed = true;
	      for (size_t r = 0; r < m1.size(); ++r)
		{
		  m1[r] -= X[r][c];
		  X[r][c] = 0;
		}
	      m2[c] = 0;
	    }
	}
    }
}


void
trymatch2(TokenIndex& V1, // source language vocab
	  TokenIndex& V2, // target language vocab
	  string const& source, // source phrase
	  string const& target, // observed target candidate
 	  vector<PhrasePair<Token> > const* const tcands,
	  vector<vector<int> >& X) // destination alignment matrix
	  // tcands: translations for source
{
  UnicodeString S,T;
  vector<int> t2p, s2p; // maps from character position in string to word pos.
  vector<int> wlen_t, wlen_s; // individual word lengths
  size_t slen = mapstring(source, S, s2p, &wlen_s);
  size_t tlen = mapstring(target, T, t2p, &wlen_t);

  X.assign(slen,vector<int>(tlen,0));
  if (slen == 1 && tlen ==1 && S == T)
    X[0][0] = S.length();
  else
    {
      align_letters(S,s2p,T,t2p,X);
      if (tcands) trymatch3(*tcands, T, tlen, t2p, V2, X);
    }

  minmatch_filter(X, wlen_s, wlen_t);
  bool hit = false;
  for (size_t r = 0; !hit && r < X.size(); ++r)
    for (size_t c = 0; !hit && c < X[r].size(); ++c)
      hit = X[r][c] > min(S.length(),T.length())/2;

  // if (hit)
  //   {
  //     cout << source << " ::: " << target;
  //     for (size_t r = 0; r < X.size(); ++r)
  // 	for (size_t c = 0; c < X[r].size(); ++c)
  // 	  cout << boost::format(" %u-%u:%d") %  r % c % X[r][c];
  //     cout << endl;
  //   }
}



// float
// trymatch(string const a, string const b,
// 	 vector<PhrasePair<Token> > const* atrans,
// 	 vector<PhrasePair<Token> > const* btrans)
// {
//   if (a == b) return a.size();
//   float score = 0;
//   float bar = lcs(a,b);
//   // score = max(bar/min(a.size(),b.size()),score);
//   score = max(bar,score);
//   // cout << "\n[" << bar << "] " << a << " ::: " << b << endl;
//   if (atrans)
//     {
//       BOOST_FOREACH(PhrasePair<Token> const& pp, *atrans)
// 	{
// 	  // if (!pp.aln.size()) continue;
// 	  ushort L = pp.aln[1], R = pp.aln[1];
// 	  for (size_t k = 3; k < pp.aln.size(); k += 2)
// 	    {
// 	      if (L > pp.aln[k]) L = pp.aln[k];
// 	      if (R < pp.aln[k]) R = pp.aln[k];
// 	    }
// 	  if (L || R+1U < pp.len2) continue;
// 	  string foo = toString(*BT.V2,pp.start2,pp.len2);
// 	  // float bar = float(lcs(foo,b))/min(foo.size(),b.size());
// 	  float bar = float(lcs(foo,b));

// 	  if (bar > .5)
// 	    {
// 	      // score = max(pp.score * bar,score);
// 	      score = max(bar,score);
// 	      // cout << "[" << bar << "] " << foo << " ::: " << b
// 	      // << " (" << a << ") " << pp.score << endl;
// 	    }
// 	}
//     }
//   if (btrans)
//     {
//       BOOST_FOREACH(PhrasePair<Token> const& pp, *btrans)
// 	{
// 	  // if (!pp.aln.size()) continue;
// 	  ushort L = pp.aln[1], R = pp.aln[1];
// 	  for (size_t k = 3; k < pp.aln.size(); k += 2)
// 	    {
// 	      if (L > pp.aln[k]) L = pp.aln[k];
// 	      if (R < pp.aln[k]) R = pp.aln[k];
// 	    }
// 	  if (L || R+1U < pp.len2) continue;
// 	  string foo = toString(*BT.V1,pp.start2,pp.len2);
// 	  // float bar = float(lcs(a,foo))/min(a.size(),foo.size());
// 	  float bar = float(lcs(a,foo));
// 	  if (bar > .5)
// 	    {
// 	      score = max(bar,score);
// 	      // cout << "[" << bar<< "] " << a << " ::: " << foo
// 	      // << " (" << b << ") " << pp.score << endl;
// 	    }
// 	}
//     }
//   return score;
// }

struct ahyp
{
  ushort s1,s2,e1,e2;
  float score;
  bool operator<(ahyp const& o) const { return score < o.score; }
  bool operator>(ahyp const& o) const { return score > o.score; }
};

struct AlnPoint
{
  enum status { no = 0, yes = 1, maybe = -1, undef = -7 };
  float    score;
  status   state;
  AlnPoint() : score(0), state(undef) {}
};

bool overlap(span_t const& a, span_t const& b)
{
  return !(a.second <= b.first || b.second <= a.first);
}

class AlnMatrix
{
  vector<bitvector> A1,A2;  // final alignment matrix
  vector<bitvector> S1,S2;  // shadow alignment matrix
public:
  vector<bitvector*> m1,m2; // margins
  AlnMatrix(size_t const rows, size_t const cols);
  bitvector const&
  operator[](size_t const r) const
  { return A1.at(r); }

  bool
  incorporate(span_t const& rspan, span_t const& cspan,
	      vector<uchar> const& aln, bool const flip);

  size_t size() const { return A1.size(); }
};

AlnMatrix::
AlnMatrix(size_t const rows, size_t const cols)
{
  A1.assign(rows,bitvector(cols));
  S1.assign(rows,bitvector(cols));
  A2.assign(cols,bitvector(rows));
  S2.assign(cols,bitvector(rows));
  m1.assign(rows,NULL);
  m2.assign(cols,NULL);
}

bool
AlnMatrix::
incorporate(span_t const& rspan,
	    span_t const& cspan,
	    vector<uchar> const& aln,
	    bool const flip)
{
  for (size_t r = rspan.first; r < rspan.second; ++r)
    S1[r].reset();
  for (size_t c = cspan.first; c < cspan.second; ++c)
    S2[c].reset();
  if (flip)
    {
      for (size_t i = 0; i < aln.size(); i += 2)
	{
	  size_t r = rspan.first + aln[i];
	  size_t c = cspan.first + aln[i+1];
	  S1[r].set(c);
	  S2[c].set(r);
	}
    }
  else
    {
      for (size_t i = 0; i < aln.size(); i += 2)
	{
	  size_t r = rspan.first + aln[i+1];
	  size_t c = cspan.first + aln[i];
	  S1[r].set(c);
	  S2[c].set(r);
	}
    }
  // check compatibility with existing alignment
  for (size_t r = rspan.first; r < rspan.second; ++r)
    if (m1[r] && (*m1[r]) != S1[r]) return false;
  for (size_t c = cspan.first; c < cspan.second; ++c)
    if (m2[c] && (*m2[c]) != S2[c]) return false;

  // all good, add new points
  for (size_t r = rspan.first; r < rspan.second; ++r)
    if (!m1[r]) { A1[r] = S1[r]; m1[r] = &A1[r]; }
  for (size_t c = cspan.first; c < cspan.second; ++c)
    if (!m2[c]) { A2[c] = S2[c]; m2[c] = &A2[c]; }

  return true;
}

struct alink
{
  size_t r,c,m;
  bool operator<(alink const& o) const { return m < o.m; }
  bool operator>(alink const& o) const { return m > o.m; }
};

int main(int argc, char* argv[])
{
  string base = argc > 1 ? argv[1] : "crp/trn/mm/";
  string L1   = argc > 1 ? argv[2] : "de";
  string L2   = argc > 1 ? argv[3] : "en";
  BT.open(base,L1,L2);
  BT.V1->setDynamic(true);
  BT.V2->setDynamic(true);
  string line1, line2;
  npmi_scorer1<Token> scorer;
  while (getline(cin,line1) and getline(cin,line2))
    {
      cout << "\n" << line1 << "\n" << line2 << endl;
      vector<Token> snt1,snt2;
      fill_token_seq(*BT.V1,line1,snt1);
      fill_token_seq(*BT.V2,line2,snt2);
      vector<vector<SPTR<vector<PhrasePair<Token> > > > > pt1,pt2;
      vector<vector<uint64_t> > pm1,pm2;
      BT.lookup(snt1,*BT.I1,pt1,&pm1,&scorer);
      BT.lookup(snt2,*BT.I2,pt2,&pm2,&scorer);

      // build map from phrases to positions
      typedef boost::unordered_map<uint64_t, vector<span_t> >
	p2s_map_t;
      typedef p2s_map_t::iterator p2s_iter;
      p2s_map_t p2s1,p2s2;
      for (ushort i = 0; i < pm1.size(); ++i)
	for (ushort k = 0; k < pm1[i].size(); ++k)
	  p2s1[pm1[i][k]].push_back(make_pair(i,i+k+1));
      for (ushort i = 0; i < pm2.size(); ++i)
	for (ushort k = 0; k < pm2[i].size(); ++k)
	  p2s2[pm2[i][k]].push_back(make_pair(i,i+k+1));

      boost::unordered_map<uint64_t,SPTR<vector<PhrasePair<Token> > > > all1,all2;
      vector<PhrasePair<Token> > pp_all;
      for (size_t i = 0; i < pt2.size(); ++i)
	for (size_t k = 0; k < pt2[i].size(); ++k)
	  all2[pm2[i][k]] = pt2[i][k];
      for (size_t i = 0; i < pt1.size(); ++i)
	for (size_t k = 0; k < pt1[i].size(); ++k)
	  {
	    all1[pm1[i][k]] = pt1[i][k];
	    BOOST_FOREACH(PhrasePair<Token> const& pp, *pt1[i][k])
	      {
		if (pp.score < 0) break;
		if (p2s2.find(pp.p2) != p2s2.end())
		  pp_all.push_back(pp);
	      }
	  }
      sort(pp_all.begin(), pp_all.end(), greater<PhrasePair<Token> >());
      vector<int> a1(snt1.size(),-1), a2(snt2.size(),-1);

      vector<bitvector> R(snt1.size(),bitvector(snt2.size()));
      vector<bitvector> C(snt2.size(),bitvector(snt1.size()));
      vector<bitvector> myR(snt1.size(),bitvector(snt2.size()));
      vector<bitvector> myC(snt2.size(),bitvector(snt1.size()));
      vector<bitvector*> m1(snt1.size(),NULL);
      vector<bitvector*> m2(snt2.size(),NULL);

      // vector<vector<AlnPoint> > M(snt1.size(),vector<AlnPoint>(snt2.size()));
      AlnMatrix A(snt1.size(),snt2.size());
      for (size_t p = 0; p < pp_all.size(); ++p)
	{
	  PhrasePair<Token> const& pp = pp_all[p];
#if 0
	  cout << (boost::format("%30s ::: %-30s ")
		   % BT.toString(pp.p1,0).c_str()
		   % BT.toString(pp.p2,1).c_str());
	  cout << (boost::format("%.4f [%d/%d/%d]")
		   % pp.score % pp.good1 % pp.joint % pp.good2);
	  for (size_t a = 0; a < pp.aln.size(); a += 2)
	    cout << " " << int(pp.aln[a]) << "-" << int(pp.aln[a+1]);
	  cout << endl;
#endif

	  vector<span_t>& v1 = p2s1[pp.p1];
	  vector<span_t>& v2 = p2s2[pp.p2];
	  if (v1.size() == 1)
	    for (size_t i = v1[0].first; i < v1[0].second; ++i)
	      if (a1[i] < 0) a1[i] = p;
	  if (v2.size() == 1)
	    for (size_t i = v2[0].first; i < v2[0].second; ++i)
	      if (a2[i] < 0) a2[i] = p;

	  if (v1.size() == 1 && v2.size() == 1)
	    A.incorporate(v1[0],v2[0],pp.aln,pp.inverse);
	}

      for (size_t i = 0; i < A.size(); ++i)
	{
	  cout << (*BT.V1)[snt1[i].id()] << ": ";
	  for (size_t k=A[i].find_first(); k < A[i].size(); k=A[i].find_next(k))
	    cout << boost::format(" %d:%s") % k % (*BT.V2)[snt2[k].id()];
	  cout << endl;
	}



      vector<PhrasePair<Token> > const* atrans, *btrans;
      ahyp h;
      vector<ahyp> hyps;
      vector<vector<int> > L(snt1.size(),vector<int>(snt2.size(),0));
      // L: matches by letter overlap

      for (h.s1 = 0; h.s1 < a1.size(); ++h.s1)
	{
	  if (a1[h.s1] >= 0) continue;
	  ostringstream buf1;
	  for (h.e1 = h.s1; h.e1 < a1.size() && a1[h.e1] < 0; ++h.e1)
	    {
	      if (h.e1 > h.s1)
		{
		  if (pt1[h.s1].size() + h.s1 <= h.e1) break;
		  buf1 << " ";
		}
	      buf1 << (*BT.V1)[snt1[h.e1].id()];
	      atrans = pt1[h.s1].size() ? pt1[h.s1].at(h.e1-h.s1).get() : NULL;
	      for (h.s2 = 0; h.s2 < a2.size(); ++h.s2)
		{
		  ostringstream buf2;
		  if (a2[h.s2] >= 0) continue;
		  for (h.e2 = h.s2; h.e2 < a2.size() && a2[h.e2] < 0; ++h.e2)
		    {
		      if (h.e2 > h.s2)
			{
			  if (pt2[h.s2].size() + h.s2 <= h.e2) break;
			  buf2 << " ";
			}
		      buf2 << (*BT.V2)[snt2[h.e2].id()];
		      btrans = (pt2[h.s2].size()
				? pt2[h.s2].at(h.e2-h.s2).get()
				: NULL);

		      vector<vector<int> > aln;
		      trymatch2(*BT.V1, *BT.V2, buf1.str(),buf2.str(),
				atrans,aln);
		      for (size_t i = 0; i < aln.size(); ++i)
			for (size_t k = 0; k < aln[i].size(); ++k)
			  L[h.s1+i][h.s2+k] = max(L[h.s1+i][h.s2+k],aln[i][k]);
		      trymatch2(*BT.V2, *BT.V1, buf2.str(),buf1.str(),
				btrans,aln);
		      for (size_t i = 0; i < aln[0].size(); ++i)
			for (size_t k = 0; k < aln.size(); ++k)
			  L[h.s1+i][h.s2+k] = max(L[h.s1+i][h.s2+k],aln[k][i]);
		      // h.score = trymatch(buf1.str(), buf2.str(), atrans, btrans);
		      // hyps.push_back(h);
		    }
		}
	    }
	}

      vector<alink> links;

      alink x;
      for (x.r = 0; x.r < L.size(); ++x.r)
	{

	for (x.c = 0; x.c < L[x.r].size(); ++x.c)
	  {
	    x.m = L[x.r][x.c];
	    if (x.m) links.push_back(x);
	  }
	}

      sort(links.begin(),links.end(),greater<alink>());

      BOOST_FOREACH(alink& x, links)
	{
	  if (L[x.r][x.c])
	    {
	      cout << (*BT.V1)[snt1[x.r].id()] << " ::: "
		   << (*BT.V2)[snt2[x.c].id()] << " ::: "
		   << L[x.r][x.c] << endl;
	    }
	}

      // sort(hyps.begin(),hyps.end(),greater<ahyp>());
      // BOOST_FOREACH(ahyp const& h, hyps)
      // 	{
      // 	  if (h.score < .5) break;
      // 	  for (size_t i = h.s1; i <= h.e1; ++i)
      // 	    cout << i << ":" << (*BT.V1)[snt1[i].id()] << " ";
      // 	  cout << " ::: ";
      // 	  for (size_t i = h.s2; i <= h.e2; ++i)
      // 	    cout << i << ":" << (*BT.V2)[snt2[i].id()] << " ";
      // 	  cout << h.score << endl;
      // 	}

    }
}


//       for (size_t i = 0; i < pt1.size(); ++i)
// 	{
// 	  for (size_t k = 0; k < pt1[i].size(); ++k)
// 	    {
// 	      size_t d1 = 0;
// 	      bool first = true;
// 	      BOOST_FOREACH(PhrasePair<Token> const& pt, *pt1[i][k])
// 		{
// 		  TSA<Token>::tree_iterator m(BT.I2.get(),pt.start2,pt.len2);
// 		  if (pt.score < 0) break;
// 		  int left = pt.aln[1], right = pt.aln[1];
// 		  bool match = p2s2.find(m.getPid()) != p2s2.end();
// 		  if (!match)
// 		    {
// 		      for (size_t a = 3; a < pt.aln.size(); a += 2)
// 			{
// 			  if (left  > pt.aln[a]) left  = pt.aln[a];
// 			  if (right < pt.aln[a]) right = pt.aln[a];
// 			}
// 		    }
// #if 0
// 		  if (match)
// 		    {
// 		      if (first)
// 			{
// 			  cout << BT.toString(pm1[i][k],0) << endl;
// 			  first = false;
// 			}
// 		      cout << boost::format("%.4f") % pt.score << " "
// 			   << setw(5) << d1 << " " << (match ? "* " : "  ")
// 			   << toString(*BT.V2, pt.start2, pt.len2) << " ["
// 			   << pt.good1 << "/" << pt.joint << "/"
// 			   << pt.good2 << "]";
// 		      for (size_t a = 0; a < pt.aln.size(); a += 2)
// 			cout << " " << int(pt.aln[a]) << "-" << int(pt.aln[a+1]);
// 		      cout << " [" << left << ":" << right << "]" << endl;
// 		    }
// #endif
// 		  if (!match)
// 		    {
// 		      if (left == 0 && pt.len2 - right == 1)
// 			d1 += pt.joint;
// 		    }
// 		  else
// 		    {
// 		      pp_all.push_back(pt);
// 		      // pp_all.back().m1 -= d1;
// 		    }

// 		}
// 	      if (!first) cout << endl;
// 	    }

