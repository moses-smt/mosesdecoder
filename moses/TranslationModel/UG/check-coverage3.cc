// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <iostream>
#include "mm/ug_bitext.h"
#include "generic/file_io/ug_stream.h"
#include <string>
#include <sstream>
#include "mm/ug_bitext_sampler.h"

#include <boost/program_options.hpp>
#include <boost/math/distributions/binomial.hpp>

namespace po=boost::program_options;
using namespace Moses;
using namespace sapt;
using namespace std;
using namespace boost;

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef mmBitext<Token> bitext_t;

size_t topN,sample_size;
string docname;
string reference_file;
string domain_name;
string bname, L1, L2;
string ifile;

struct mycmp 
{
  bool operator() (pair<string,uint32_t> const& a, 
                   pair<string,uint32_t> const& b) const
  {
    return a.second > b.second;
  }
};



void interpret_args(int ac, char* av[]);

string 
basename(string const path)
{
  size_t p = path.find_last_of("/");
  string dot = ".";
  size_t k = path.find((dot + L1),p+1);
  if (k == string::npos) k = path.find(dot + L1 + ".gz");
  if (k == string::npos) return path.substr(p+1);
  return path.substr(p+1, k-p-1);
}

void 
print_evidence_list(bitext_t const& B, 
                    sapt::pstats::indoc_map_t const& indoc_src,
                    PhrasePair<Token> const& ppair)
{
  typedef boost::math::binomial_distribution<> binomial;

  typedef std::map<uint32_t, uint32_t>::const_iterator iter;
  typedef pair<size_t,size_t> item;

  float pfwd = float(ppair.joint)/ppair.good1;
  
  std::map<uint32_t, uint32_t> const& indoc = ppair.indoc;

  vector<item> where; 
  where.reserve(indoc.size());
  
  for (iter d = indoc.begin(); d != indoc.end(); ++d)
    where.push_back(item(d->second, d->first));
  sort(where.begin(),where.end(),greater<item>());
  BOOST_FOREACH(item const& doc, where)
    {
      if (domain_name == B.docid2name(doc.second))
        cout << (boost::format("\t\t%4d ! %s") % doc.first % B.docid2name(doc.second));
      else
        cout << (boost::format("\t\t%4d   %s") % doc.first % B.docid2name(doc.second));

      size_t N = indoc_src.find(doc.second)->second;
      float p0;
      if (doc.first > pfwd * N)
        p0 = cdf(complement(binomial(N, pfwd), doc.first));
      else
        p0 = cdf(binomial(N, pfwd), doc.first);

      // for (size_t j = 0; j < N; ++j)
      //   printf("%5d %.2f %.2f\n", int(j),
      //          cdf(complement(binomial(N, pfwd), j)),
      //          cdf(binomial(N, pfwd), j));
               
      // cout << (boost::format(" (%d: %.2f %.1f)") % N % (float(doc.first)/N) % p0) << endl;
      cout << (boost::format(" (%.1f %f)") % (pfwd * N) % p0) << endl;
    }
}

int main(int argc, char* argv[])
{
  boost::shared_ptr<bitext_t> B(new bitext_t);
  interpret_args(argc,argv);

  B->open(bname, L1, L2);
  string line, refline;
  if (domain_name == "" && ifile != "-")
    domain_name = basename(ifile);
  
  id_type docid = B->docname2docid(domain_name);
  boost::iostreams::filtering_istream in, ref;
  ugdiss::open_input_stream(ifile,in);
  if (reference_file.size()) 
    ugdiss::open_input_stream(reference_file,ref);

  while(getline(in,line))
    {
      if (reference_file.size()) getline(ref, refline);
      cout << string(80,'-') << endl;
      cout << " [" << domain_name << "]" << endl;
      cout << line << endl;
      if (refline.size()) cout << refline << endl;
      cout << string(80,'-') << endl;
      vector<id_type> snt;
      B->V1->fillIdSeq(line,snt);
      for (size_t i = 0; i < snt.size(); ++i)
        {
          bitext_t::iter m(B->I1.get());
          for (size_t k = i; k < snt.size() && m.extend(snt[k]); ++k);
          for (size_t num_occurrences = 0; m.size(); m.up())
            {
              if (size_t(m.ca()) == num_occurrences) continue;
              num_occurrences = m.ca();
              // num_occurrences = m.rawCnt();
              SPTR<SamplingBias const> zilch;
              BitextSampler<Token> s(B, m, zilch, sample_size, sample_size, 
                                     sapt::random_sampling);
              s();
              if (s.stats()->trg.size() == 0) continue;
              sapt::pstats::indoc_map_t::const_iterator d
                = s.stats()->indoc.find(docid);
              size_t indoccnt = d != s.stats()->indoc.end() ? d->second : 0;
              cout << m.str(B->V1.get()) << " (" 
                   << s.stats()->trg.size() << " entries; " 
                   << indoccnt << "/" << s.stats()->good 
                   << " samples in domain; " << num_occurrences
                   << " occ.)" << endl;
              vector<PhrasePair<Token> > ppairs;
              PhrasePair<Token>::SortDescendingByJointCount sorter;
              expand(m,*B,*s.stats(),ppairs,NULL);
              sort(ppairs.begin(),ppairs.end(),sorter);
              boost::format fmt("%4d/%d/%d |%s| (%4.2f : %4.2f)"); 
              size_t ctr = 0;
              bool skipped_some = false;
              BOOST_FOREACH(PhrasePair<Token>& ppair, ppairs)
                {
                  if (++ctr > topN && ppair.indoc.find(docid) == ppair.indoc.end())
                    {
                      skipped_some = true;
                      continue;
                    }
                  if (skipped_some) 
                    {
                      cout << string(17,' ') << "..." << endl;
                      skipped_some = false;
                    }
                  // if (ppair.joint * 100 < ppair.good1) break;
                  ppair.good2 = ppair.raw2 * float(ppair.good1)/ppair.raw1;
                  ppair.good2 = max(ppair.good2, ppair.joint);

#if 1
                  cout << "\t" 
                       << (fmt % ppair.joint % ppair.good1 % ppair.good2
                           % B->T2->pid2str(B->V2.get(),ppair.p2)
                           % (float(ppair.joint)/ppair.good1)
                           % (float(ppair.joint)/ppair.good2)
                           ) << "\n";
                  print_evidence_list(*B, s.stats()->indoc, ppair);
                  cout << endl;
#else
                  cout << "\t" 
                       << (fmt % ppair.joint % ppair.good1 % ppair.good2
                           % B->T2->pid2str(B->V2.get(),ppair.p2)
                           % (float(ppair.joint)/ppair.good1)
                           % (float(ppair.joint)/ppair.good2)
                           ) << " [";
                  typedef std::map<uint32_t, uint32_t>::const_iterator iter;
                  for (iter d = ppair.indoc.begin(); d != ppair.indoc.end(); ++d)
                    {
                      if (d != ppair.indoc.begin()) cout << "; ";
                      cout << (boost::format("%s: %d") % B->docid2name(d->first)
                               % d->second) ;
                    }
                  cout << "]" << endl;

#endif

                }
            }
        }
    }
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()

    ("help,h",  "print this message")
    ("top,t", po::value<size_t>(&topN)->default_value(5),
     "max. number of entries to show")
    ("sample,N", po::value<size_t>(&sample_size)->default_value(1000),
     "sample size")
    ("domain,D", po::value<string>(&domain_name),
     "domain name (when reading from stdin)")
    ("reference,r", po::value<string>(&reference_file),
     "reference file")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("L1", po::value<string>(&L1), "L1 tag")
    ("L2", po::value<string>(&L2), "L2 tag")
    ("input", po::value<string>(&ifile), "input file")
    ;

  h.add(o);
  po::positional_options_description a;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);
  a.add("input",1);

  po::store(po::command_line_parser(ac,av)
            .options(h)
            .positional(a)
            .run(),vm);
  po::notify(vm);
  if (vm.count("help"))
    {
      std::cout << "\nusage:\n\t" << av[0]
                << " [options] <model file stem> <L1> <L2> <input file>" << std::endl;
      std::cout << o << std::endl;
      exit(0);
    }
}
