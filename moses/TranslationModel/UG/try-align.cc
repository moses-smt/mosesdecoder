#include "mm/ug_bitext.h"
#include <boost/format.hpp>
using namespace std;
using namespace Moses;
using namespace ugdiss;
using namespace sapt;

typedef L2R_Token<SimpleWordId> Token;
typedef mmTtrack<Token> ttrack_t;
typedef mmTSA<Token>     tsa_t;

TokenIndex V1,V2;
boost::shared_ptr<ttrack_t> T1,T2;
tsa_t I1,I2;

float lbop_level = .05;
#define smooth 1
namespace stats
{
  using namespace Moses;
  using namespace sapt;
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


struct PPair
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

  PPair(ushort s1_=0, ushort e1_=0, ushort s2_=0, ushort e2_=0)
    : s1(s1_), e1(e1_), s2(s2_), e2(e2_), parent(-1) { }


  bool
  operator<(PPair const& other) const
  {
    return (this->stats.score == other.stats.score
	    ? (e1-s1 + e2-s2 > other.e1-other.s1 + other.e2-other.s2)
	    : (this->stats.score > other.stats.score));
  }

  size_t len1() const { return e1 - s1; }
  size_t len2() const { return e2 - s2; }
  bool includes(PPair const& o) const
  {
    return s1 <= o.s1 && e1 >= o.e1 && s2 <= o.s2 && e2 >= o.e2;
  }

};

SinglePhrase::cache_t cache1,cache2;
PPair::stats_t::cache_t ppcache;


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

int main(int argc, char* argv[])
{
  string base = argv[1];
  string L1   = argv[2];
  string L2   = argv[3];

  T1.reset(new ttrack_t());
  T2.reset(new ttrack_t());

  V1.open(base + L1 + ".tdx");
  T1->open(base + L1 + ".mct");
  I1.open(base + L1 + ".sfa", T1);

  V2.open(base + L2 + ".tdx");
  T2->open(base + L2 + ".mct");
  I2.open(base + L2 + ".sfa", T2);

  tsa_t::tree_iterator m1(&I1);
  tsa_t::tree_iterator m2(&I1);
  string line1, line2;
  while (getline(cin,line1) and getline(cin,line2))
    {
      cout << "\n" << line1 << "\n" << line2 << endl;
      vector<vector<SPTR<SinglePhrase> > > M1,M2;
      vector<id_type> snt1,snt2;
      V1.fillIdSeq(line1,snt1);
      V2.fillIdSeq(line2,snt2);
      lookup_phrases(snt1,V1,*T1,I1,cache1,M1);
      lookup_phrases(snt2,V2,*T2,I2,cache2,M2);

      vector<PPair> pp_all, pp_good;
      vector<int> a1(snt1.size(),-1);
      vector<int> a2(snt2.size(),-1);

      vector<vector<int> > z1(snt1.size(),vector<int>(snt1.size(),-1));
      vector<vector<int> > z2(snt2.size(),vector<int>(snt2.size(),-1));
      vector<vector<vector<PPair> > >ppm1(M1.size()),ppm2(M2.size());
      vector<vector<float> >  M(snt1.size(), vector<float>(snt2.size(),0));
      vector<vector<size_t> > best1(snt1.size()), best2(snt2.size());
      for (size_t i1 = 0; i1 < M1.size(); ++i1)
	{
	  PPair pp;
	  pp.s1 = i1;
	  ppm1[i1].resize(M1[i1].size());
	  for (size_t i2 = 0; i2 < M2.size(); ++i2)
	    {
	      pp.s2 = i2;
	      pp.stats.j = 1;
	      ppm2[i2].resize(M2[i2].size());
	      for (size_t k1 = 0; k1 < M1[i1].size(); ++k1)
		{
		  pp.e1 = i1 + k1 + 1;
		  // if (pp.stats.j == 0) break;
		  for (size_t k2 = 0; k2 < M2[i2].size(); ++k2)
		    {
		      pp.e2 = i2 + k2 + 1;
		      SPTR<PPair::stats_t> & s
			= ppcache[make_pair(M1[i1][k1]->pid,M2[i2][k2]->pid)];
		      if (!s)
			{
			  s.reset(new PPair::stats_t());
			  s->set(M1[i1][k1]->occs,M2[i2][k2]->occs,T1->size());
			}
		      pp.stats = *s;
		      if (pp.stats.j == 0) break;
		      // ppm1[i1][k1].push_back(pp);
		      // ppm2[i2][k2].push_back(pp);
		      size_t J = pp.stats.j * 100;
		      if (pp.stats.score > 0
			  && J >= pp.stats.m1
			  && J > pp.stats.m2)
			{ pp_all.push_back(pp); }
		    }
		}
	    }
	}
      sort(pp_all.begin(),pp_all.end());
#if 0
      BOOST_FOREACH(PPair const& pp,pp_all)
	{
	  if (pp.stats.npmi < 0) continue;
	  for (size_t r = pp.s1; r < pp.e1; ++r)
	    for (size_t c = pp.s2; c < pp.e2; ++c)
	      {
		// M[r][c] += log(1-pp.stats.npmi);
		M[r][c] += log(1-pp.stats.mi);
	      }
	}
      for (size_t r = 0; r < M.size(); ++r)
	for (size_t c = 0; c < M[r].size(); ++c)
	  M[r][c] = 1.-exp(M[r][c]);
      for (size_t r = 0; r < best1.size(); ++r)
	{
	  best1[r].resize(snt2.size());
	  for (size_t c = 0; c < best1[r].size(); ++c)
	    best1[r][c] = c;
	  sort(best1[r].begin(),best1[r].end(),ColIndexSorter(M,r));
	}
      for (size_t c = 0; c < best2.size(); ++c)
	{
	  best2[c].resize(snt1.size());
	  for (size_t r = 0; r < best2[c].size(); ++r)
	    best2[c][r] = r;
	  sort(best2[c].begin(),best2[c].end(),RowIndexSorter(M,c));
	}
      for (size_t r = 0; r < best1.size(); ++r)
	{
	  cout << V1[snt1[r]] << ":";
	  for (size_t i = 0; i < min(3UL,M[r].size()); ++i)
	    {
	      size_t k = best1[r][i];
	      // if (M[r][k] >= M[best2[k][min(2UL,M.size())]][k])
	      cout << " " << k << ":" << V2[snt2[k]] << " " << M[r][k];
	    }
	  cout << endl;
	}
#endif
#if 0
      for (size_t k = 1; k < pp_all.size(); ++k)
	for (size_t i = k; i--;)
	  if (pp_all[i].s1 >= pp_all[k].s1 &&
	      pp_all[i].e1 <= pp_all[k].e1 &&
	      pp_all[i].s2 >= pp_all[k].s2 &&
	      pp_all[i].e2 <= pp_all[k].e2)
	    pp_all[i].stats.score += pp_all[k].stats.score;
      sort(pp_all.begin(),pp_all.end());
#endif

#if 1
      vector<int> assoc1(snt1.size(),-1), assoc2(snt2.size(),-1);
      for (size_t p = 0; p < pp_all.size(); ++p)
	{
	  PPair const& x = pp_all[p];
	  // if (x.stats.npmi < .7) break;
	  // if (z1[x.s1][x.e1-1] >= 0 || z2[x.s2][x.e2-1] >=0)
	  // continue;
	  for (size_t i = x.s1; i < x.e1; ++i)
	    {
	      if (assoc1[i] < 0)
		assoc1[i] = p;
	      else
		{
		  // PPair& y = pp_all[assoc1[i]];
		  // if (y.includes(x))
		  // assoc1[i] = p;
		}
	    }
	  for (size_t i = x.s2; i < x.e2; ++i)
	    {
	      if (assoc2[i] < 0)
		assoc2[i] = p;
	      else
		{
		  // PPair& y = pp_all[assoc2[i]];
		  // if (y.includes(x))
		    // assoc2[i] = p;
		}
	    }
	  z1[x.s1][x.e1-1] = p;
	  z2[x.s2][x.e2-1] = p;
	  continue;
	  cout << (boost::format("%.4f %.8f %.4f")
		   % x.stats.score
		   % x.stats.mi
		   % x.stats.npmi);
	  for (size_t z = x.s1; z < x.e1; ++z)
	    cout << " " << V1[snt1[z]];
	  cout << " :::";
	  for (size_t z = x.s2; z < x.e2; ++z)
	    cout << " " << V2[snt2[z]];
	  cout << " ["
	       << x.stats.m1 << "/" << x.stats.j << "/" << x.stats.m2
	       << "]" << endl;
	}
      vector<bool> done(pp_all.size(),false);
      for (size_t i = 0; i < snt1.size(); ++i)
	{
	  if (assoc1[i] < 0 || done[assoc1[i]])
	    continue;
	  // for (size_t k = 0; k < snt2.size(); ++k)
	    // if (assoc1[i] == assoc2[k])
	      {
		done[assoc1[i]] = true;
		PPair& p = pp_all[assoc1[i]];
		for (size_t j = p.s1; j < p.e1; ++j)
		  cout << j << ":" << V1[snt1[j]] << " ";
		cout << " ::: ";
		for (size_t j = p.s2; j < p.e2; ++j)
		  cout << j << ":" << V2[snt2[j]] << " ";
		cout << "["
		     << p.stats.m1 << "/" << p.stats.j << "/" << p.stats.m2
		     << "] "<< p.stats.score << endl;
		// break;
	      }
	}
      cout << endl;
      for (size_t i = 0; i < snt2.size(); ++i)
	{
	  if (assoc2[i] < 0 || done[assoc2[i]])
	    continue;
	  done[assoc2[i]] = true;
	  PPair& p = pp_all[assoc2[i]];
	  for (size_t j = p.s1; j < p.e1; ++j)
	    cout << j << ":" << V1[snt1[j]] << " ";
	  cout << " ::: ";
	  for (size_t j = p.s2; j < p.e2; ++j)
	    cout << j << ":" << V2[snt2[j]] << " ";
	  cout << "["
	       << p.stats.m1 << "/" << p.stats.j << "/" << p.stats.m2
	       << "] "<< p.stats.score << endl;
	}
#endif
      // sort(pp_all.begin(),pp_all.end());
      // BOOST_FOREACH(PPair const& pp, pp_all)
      // 	{
      // 	  while (ppm1[pp.s1].size() < pp.e1 - pp.s1)
      // 	    ppm1[pp.s1].push_back(vector<PPair>());
      // 	  vector<PPair>& v1 = ppm1[pp.s1][pp.e1-pp.s1-1];
      // 	  if (v1.size() && v1[0].stats.score > pp.stats.score)
      // 	    continue;
      // 	  while (ppm2[pp.s2].size() < pp.e2 - pp.s2)
      // 	    ppm2[pp.s2].push_back(vector<PPair>());
      // 	  vector<PPair>& v2 = ppm2[pp.s2][pp.e2-pp.s2-1];
      // 	  if (v2.size() && v2[0].stats.score > pp.stats.score)
      // 	    continue;
      // 	  v1.push_back(pp);
      // 	  v2.push_back(pp);
      // 	}


      // BOOST_FOREACH(vector<vector<PPair> >& vv, ppm1)
      // 	{
      // 	  BOOST_FOREACH(vector<PPair>& v, vv)
      // 	    {
      // 	      sort(v.begin(),v.end());
      // 	      if (v.size() > 1 && v[0].stats.score == v[1].stats.score)
      // 		v.clear();
      // 	    }
      // 	}
      // for (size_t i2 = 0; i2 < ppm2.size(); ++i2)
      // 	{
      // 	  for (size_t k2 = 0; k2 < ppm2[i2].size(); ++k2)
      // 	    {
      // 	      vector<PPair>& v2 = ppm2[i2][k2];
      // 	      sort(v2.begin(),v2.end());
      // 	      if (v2.size() > 1 && v2[0].stats.score == v2[1].stats.score)
      // 		{
      // 		  v2.clear();
      // 		  continue;
      // 		}
      // 	      ushort i1 = v2[0].s1;
      // 	      ushort k1 = v2[0].e1 - i1 -1;

      // 	      if (ppm1[i1][k1].size() == 0 ||
      // 		  ppm1[i1][k1][0].s2 != i2 ||
      // 		  ppm1[i1][k1][0].e2 != i2 + k2 + 1)
      // 		{ v2.clear(); }
      // 	      else pp_good.push_back(ppm2[i2][k2][0]);
      // 	    }
      // 	}
      // BOOST_FOREACH(PPair const& pp, pp_good)
      // 	{
      // 	  cout << pp.stats.mi << " ";
      // 	  for (size_t z = pp.s1; z < pp.e1; ++z)
      // 	    cout << V1[snt1[z]] << " ";
      // 	  cout << " ::: ";
      // 	  for (size_t z = pp.s2; z < pp.e2; ++z)
      // 	    cout << V2[snt2[z]] << " ";
      // 	  cout << pp.stats.m1 << "/" << pp.stats.j << "/" << pp.stats.m2 << endl;
      // 	}
      // // cout << string(80,'=') << endl;
      // // sort(pp_all.begin(),pp_all.end());
      // // BOOST_FOREACH(PPair const& pp, pp_all)
      // // 	{
      // // 	  cout << pp.mi << " ";
      // // 	  for (size_t z = pp.s1; z < pp.e1; ++z)
      // // 	    cout << V1[snt1[z]] << " ";
      // // 	  cout << " ::: ";
      // // 	  for (size_t z = pp.s2; z < pp.e2; ++z)
      // // 	    cout << V2[snt2[z]] << " ";
      // // 	  cout << pp.m1 << "/" << pp.j << "/" << pp.m2 << endl;
      // // 	}

    }
}

