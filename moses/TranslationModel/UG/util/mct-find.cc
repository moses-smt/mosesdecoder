#include <boost/program_options.hpp>
#include <boost/unordered_set.hpp>
#include "mm/ug_bitext.h"
#include "generic/sorting/NBestList.h"
#include <string>
#include <sstream>

using namespace std;
using namespace Moses;
using namespace sapt;

namespace po=boost::program_options;
typedef L2R_Token<SimpleWordId> Token;
typedef mmBitext<Token> mmbitext;
typedef Bitext<Token>::tsa tsa;

class NBestList2
{
  vector<int>     m_index;
  vector<id_type>  m_heap;
  mutable vector<id_type> m_nbest;
  
  void 
  swap_heap_elements(size_t const i, size_t const k)
  {
    swap(m_index[m_heap[i]], m_index[m_heap[k]]);
    swap(m_heap[i], m_heap[k]);
  }

  void 
  update_heap(size_t i)
  {
    // lowest score should be on top of heap
    if (i && score[m_heap[i]] < score[m_heap[i/2]])
      { // bubble up
	while (i && score[m_heap[i]] < score[m_heap[i/2]])
	  {
	    // cerr << i << endl;
	    swap_heap_elements(i, i/2);
	    i /= 2;
	  }
	return;
      }
    
    for (size_t k = (i+1)*2-1; k < m_heap.size(); k = (i+1)*2 - 1)
      { // bubble down
	if (k+1 < m_heap.size() && score[m_heap[k+1]] < score[m_heap[k]]) ++k;
	if (score[m_heap[i]] > score[m_heap[k]])
	  {
	    // cerr << k << endl;
	    swap_heap_elements(i,k);
	    i = k;
	  }
	else return;
      }
  }


public:
  vector<float> score;

  NBestList2(size_t const num_sents, size_t const nbest_list_size) 
    : score(num_sents,  0)
    , m_index(num_sents, -1)
  {
    m_heap.reserve(nbest_list_size);
  }

  void
  consider(uint32_t const sid)
  {
    if (m_nbest.size()) m_nbest.clear();
    if (m_index[sid] < 0)
      {
	if (m_heap.size() == m_heap.capacity())
	  {
	    if (score[sid] <= score[m_heap[0]]) 
	      return;
	    m_index[m_heap[0]] = -1;
	    m_index[sid] = 0;
	    m_heap[0] = sid;
	  }
	else 
	  {
	    m_index[sid] = m_heap.size();
	    m_heap.push_back(sid);
	  }
      }
    update_heap(m_index[sid]);
  }

  void
  update(uint32_t const sid, float localscore)
  {
    score[sid] += localscore;
    consider(sid);
  }


  size_t size() const { return m_heap.size(); }

  id_type 
  operator[](id_type const i) const 
  {
    UTIL_THROW_IF2(i >= m_heap.size(),"index out of range");
    if (m_nbest.empty())
      {
	m_nbest = m_heap;
	VectorIndexSorter<float,greater<float>,id_type> sorter(score);
	sort(m_nbest.begin(), m_nbest.end(), sorter);
      }
    return m_nbest[i];
  }

  void reset() 
  {
    BOOST_FOREACH(id_type const& i, m_heap) m_index[i] = -1;
    m_heap.clear();
  }

};


int main(int argc, char* argv[])
{
  SPTR<mmTtrack<Token> > C(new mmTtrack<Token>);
  TokenIndex      V;
  mmTSA<Token>    I; 
  string bname = argv[1];
  C->open(bname + ".mct");
  V.open(bname + ".tdx");
  I.open(bname + ".sfa", C);
  string line;
  size_t nbest_list_size = 20;
  boost::unordered_set<uint64_t> done;
  NBestList2 nbest_list(C->size(), nbest_list_size);
  vector<vector<id_type> > doc;
  while (getline(cin,line))
    { 
      doc.push_back(V.toIdSeq(line));
      vector<short> frontier(C->size(),-1);
      vector<id_type> const& snt = doc.back();
      short stop = snt.size();
      for (short i = 0; i < stop; ++i)
	{
	  mmTSA<Token>::tree_iterator m(&I);
	  for (size_t k = i; k < snt.size() && m.extend(snt[k]); ++k);
	  if (m.size() == 0 || m.ca() >= 1000) continue; 
	  size_t ca = 0;
	  for (;m.size() && m.ca() < 1000; m.up())
	    {
	      if (done.insert(m.getPid()).second == false) break;
	      if (size_t(m.ca()) == ca) continue;
	      ca = m.ca();
	      // cout << m.str(&V) << " " << ca << endl;
	      tsa::ArrayEntry A(m.lower_bound(-1));
	      while (A.next != m.upper_bound(-1))
		{
		  m.root->readEntry(A.next, A);
		  if (frontier[A.sid] >= short(i + m.size())) continue;
		  frontier[A.sid] = i + m.size();
		  nbest_list.score[A.sid] += 1./(m.ca() * C->sntLen(A.sid));
		}
	    }
	}
    }

  done.clear();
  BOOST_FOREACH(vector<id_type> const& snt, doc)
    {
      short stop = snt.size();
      for (short i = 0; i < stop; ++i)
	{
	  mmTSA<Token>::tree_iterator m(&I);
	  for (size_t k = i; k < snt.size() && m.extend(snt[k]); ++k);
	  size_t ca = 0;
	  for (;m.size();  m.up())
	    {
	      if (done.insert(m.getPid()).second == false) break;
	      if (m.size() == 0 || size_t(m.ca()) == ca) continue;
	      ca = m.ca();
	      cout << "\n" << m.str(&V) << " " << ca << endl;
	      nbest_list.reset();
	      tsa::ArrayEntry A(m.lower_bound(-1));
	      while (A.next != m.upper_bound(-1))
		{
		  m.root->readEntry(A.next, A);
		  nbest_list.consider(A.sid);
		}
	      for (size_t i = 0; i < nbest_list.size(); ++i)
		{
		  size_t cand = nbest_list[i];
		  cout << setw(4) << i << " " 
		       << nbest_list.score[cand] << " " 
		       << toString(V, C->sntStart(cand), C->sntLen(cand)) << endl;
		}
	    }
	}
    }
}

// void
// interpret_args(int ac, char* av[])
// {
//   po::variables_map vm;
//   po::options_description o("Options");
//   o.add_options()

//     ("help,h",  "print this message")
//     ("maxhits,n", po::value<size_t>(&maxhits)->default_value(25),
//      "max. number of hits")
//     ("q1", po::value<string>(&Q1), "query in L1")
//     ("q2", po::value<string>(&Q2), "query in L2")
//     ;

//   po::options_description h("Hidden Options");
//   h.add_options()
//     ("bname", po::value<string>(&bname), "base name of corpus")
//     ("L1", po::value<string>(&L1), "L1 tag")
//     ("L2", po::value<string>(&L2), "L2 tag")
//     ;

//   h.add(o);
//   po::positional_options_description a;
//   a.add("bname",1);
//   a.add("L1",1);
//   a.add("L2",1);

//   po::store(po::command_line_parser(ac,av)
//             .options(h)
//             .positional(a)
//             .run(),vm);
//   po::notify(vm);
//   if (vm.count("help"))
//     {
//       std::cout << "\nusage:\n\t" << av[0]
//            << " [options] [--q1=<L1string>] [--q2=<L2string>]" << std::endl;
//       std::cout << o << std::endl;
//       exit(0);
//     }
// }
