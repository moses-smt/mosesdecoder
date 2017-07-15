#include <boost/program_options.hpp>
#include "mm/ug_bitext.h"
#include <string>

using namespace std;
using namespace Moses;
using namespace sapt;

namespace po=boost::program_options;
typedef L2R_Token<SimpleWordId> Token;
typedef mmBitext<Token> mmbitext;
typedef Bitext<Token>::tsa tsa;

string bname, L1, L2, Q1, Q2;
size_t maxhits;
void interpret_args(int ac, char* av[]);


void
write_sentence
(Ttrack<Token> const& T, uint32_t const sid, TokenIndex const& V, ostream& out)
{
  Token const* t = T.sntStart(sid);
  Token const* e = T.sntEnd(sid);
  // size_t i = 0;
  while (t < e)
    {
      // out << i++ << ":";
      out << V[t->id()];
      if (++t < e) out << " ";
    }
}

bool
fill(string const& query, TSA<Token> const& tsa,
     TokenIndex const& V, bitvector& v)
{
  v.resize(tsa.getCorpus()->size());
  Bitext<Token>::iter m(&tsa);
  istringstream buf(query); string w;
  while (buf >> w)
    if (!m.extend(V[w]))
      return false;
  m.markSentences(v);
  return true;
}




int main(int argc, char* argv[])
{
  interpret_args(argc, argv);
  if (Q1.empty() && Q2.empty()) exit(0);

  boost::shared_ptr<mmbitext> B(new mmbitext); string w;
  B->open(bname, L1, L2);

  Bitext<Token>::iter m1(B->I1.get(), *B->V1, Q1);
  if (Q1.size() && m1.size() == 0) exit(0);

  Bitext<Token>::iter m2(B->I2.get(), *B->V2, Q2);
  if (Q2.size() && m2.size() == 0) exit(0);

  bitvector check(B->T1->size());
  if (Q1.size() == 0 || Q2.size() == 0) check.set();
  else (m2.markSentences(check));

  Bitext<Token>::iter& m = m1.size() ? m1 : m2;
  char const* x = m.lower_bound(-1);
  char const* stop = m.upper_bound(-1);
  uint64_t sid;
  ushort off;
  boost::taus88 rnd;
  size_t N = m.approxOccurrenceCount();
  maxhits = min(N, maxhits);
  size_t k = 0; // selected
  for (size_t i = 0; x < stop; ++i)
    {
      x = m.root->readSid(x,stop,sid);
      x = m.root->readOffset(x,stop,off);

      if (!check[sid]) continue;
      size_t r = (N - i) * rnd()/(rnd.max()+1.) + k;
      if (maxhits != N && r >= maxhits) continue;
      ++k;

      size_t s1,s2,e1,e2; int po_fwd=-1,po_bwd=-1;
      std::vector<unsigned char> caln;
      // cout << sid  << " " << B->docname(sid) << std::endl;
      if (!B->find_trg_phr_bounds(sid, off, off+m.size(),
				 s1,s2,e1,e2,po_fwd,po_bwd,
				 &caln, NULL, &m == &m2))
	{
	  // cout << "alignment failure" << std::endl;
	}

      std::cout << sid  << " " << B->sid2docname(sid)
		<< " dfwd=" << po_fwd << " dbwd=" << po_bwd
		<< "\n";
      
      write_sentence(*B->T1, sid, *B->V1, std::cout); std::cout << "\n";
      write_sentence(*B->T2, sid, *B->V2, std::cout); std::cout << "\n";
      B->write_yawat_alignment(sid,
			       m1.size() ? &m1 : NULL,
			       m2.size() ? &m2 : NULL, std::cout);
      std::cout << std::endl;

    }
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()

    ("help,h",  "print this message")
    ("maxhits,n", po::value<size_t>(&maxhits)->default_value(25),
     "max. number of hits")
    ("q1", po::value<string>(&Q1), "query in L1")
    ("q2", po::value<string>(&Q2), "query in L2")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("L1", po::value<string>(&L1), "L1 tag")
    ("L2", po::value<string>(&L2), "L2 tag")
    ;

  h.add(o);
  po::positional_options_description a;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);

  po::store(po::command_line_parser(ac,av)
            .options(h)
            .positional(a)
            .run(),vm);
  po::notify(vm);
  if (vm.count("help"))
    {
      std::cout << "\nusage:\n\t" << av[0]
           << " [options] [--q1=<L1string>] [--q2=<L2string>]" << std::endl;
      std::cout << o << std::endl;
      exit(0);
    }
}
