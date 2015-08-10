#include "moses/TranslationModel/UG/mm/ug_mm_ttrack.h"
#include "moses/TranslationModel/UG/mm/ug_mm_tsa.h"
#include "moses/TranslationModel/UG/mm/tpt_tokenindex.h"
#include "moses/TranslationModel/UG/mm/ug_corpus_token.h"
#include "moses/TranslationModel/UG/mm/ug_typedefs.h"
#include "moses/TranslationModel/UG/mm/tpt_pickler.h"
#include "moses/TranslationModel/UG/mm/ug_bitext.h"
#include "moses/TranslationModel/UG/mm/ug_lexical_phrase_scorer2.h"

#include "moses/TranslationModel/UG/generic/file_io/ug_stream.h"

// using namespace Moses;
using namespace ugdiss;
using namespace sapt;
using namespace std;

typedef L2R_Token<SimpleWordId> Token;
TokenIndex V;
SPTR<vector<vector<Token> > > C(new vector<vector<Token> >());
void
add_file(string fname)
{
  boost::iostreams::filtering_istream in;
  open_input_stream(fname,in);
  string line;
  while (getline(in,line))
    {
      C->push_back(vector<Token>());
      fill_token_seq(V,line,C->back());
    }
}

int
main(int argc, char* argv[])
{
  V.setDynamic(true);
  add_file(argv[1]);
  SPTR<imTtrack<Token> > T(new imTtrack<Token>(C));
  imTSA<Token> I(T,NULL,NULL);
  string line;
  while (getline(cin,line))
    {
      vector<Token> seq; fill_token_seq<Token>(V,line,seq);
      for (size_t i = 0; i < seq.size(); ++i)
	{
	  TSA<Token>::tree_iterator m(&I);
	  cout << V[seq[i].id()];
	  for (size_t k = i; k < seq.size() && m.extend(seq[k]); ++k)
	    {
	      cout << " ";
	      if (k > i) cout << V[seq[k].id()] << " ";
	      cout << "[" << m.approxOccurrenceCount() << "]";
	    }
	  cout << endl;
	}
    }
}
