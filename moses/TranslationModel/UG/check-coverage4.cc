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

#include "LSA.h"

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
string dalsa_path;
size_t dalsa_cols;
size_t prune;

vector<float> const&
operator+=(vector<float>& a, vector<float> const& b)
{
  if (a.size() != b.size()) throw "Can't add vectors of different length";
  for (size_t i = 0; i < a.size(); ++i)
    a[i] += b[i];
  return a;
}

vector<float> const&
operator-=(vector<float>& a, vector<float> const& b)
{
  if (a.size() != b.size()) throw "Can't add vectors of different length";
  for (size_t i = 0; i < a.size(); ++i)
    a[i] -= b[i];
  return a;
}

vector<float> const&
operator+=(vector<float>& a, ug::mmTable::SubTable<float, 1u> const& b)
{
  if (a.size() != b.size()) throw "Can't add vectors of different length";
  for (size_t i = 0; i < a.size(); ++i)
    a[i] += b[i];
  return a;
}

vector<float> const&
operator-=(vector<float>& a, ug::mmTable::SubTable<float, 1u> const& b)
{
  if (a.size() != b.size()) throw "Can't add vectors of different length";
  for (size_t i = 0; i < a.size(); ++i)
    a[i] -= b[i];
  return a;
}


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
               
      cout << (boost::format(" (%d: %.2f %.1f)") % N % (float(doc.first)/N) % p0) << endl;
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

  Moses::LsaModel LSA;
  LSA.open(dalsa_path, L1, L2, dalsa_cols);
  
  id_type docid = B->docname2docid(domain_name);
  boost::iostreams::filtering_istream in, ref;
  ugdiss::open_input_stream(ifile,in);
  if (reference_file.size()) 
    ugdiss::open_input_stream(reference_file,ref);

  while(getline(in,line))
    {
      Moses::LsaTermMatcher matcher;
      matcher.fold_in(&LSA, line);
      
      if (reference_file.size()) getline(ref, refline);
      // cout << string(80,'-') << endl;
      // cout << " [" << domain_name << "]" << endl;
      // cout << line << endl;
      // if (refline.size()) cout << refline << endl;
      // cout << string(80,'-') << endl;
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
              vector<PhrasePair<Token> > ppairs;
              PhrasePair<Token>::SortDescendingByJointCount sorter;
              expand(m,*B,*s.stats(),ppairs,NULL);
              sort(ppairs.begin(),ppairs.end(),sorter);
              boost::format fmt("%4d/%d/%d |%s| (%4.2f : %4.2f)"); 
              size_t ctr = 0;
              bool skipped_some = false;
              cout << string(80,'-') << endl;
              cout << line << endl;
              if (refline.size()) cout << refline << endl;
              cout << string(80,'-') << endl;
              
              cout << m.str(B->V1.get()) << " (" 
                   << s.stats()->trg.size() << " entries; " 
                   << indoccnt << "/" << s.stats()->good 
                   << " samples in domain; " << num_occurrences
                   << " occ.)" << endl;

              for (size_t i = 0; i < ppairs.size(); ++i)
                {
                  
                  PhrasePair<Token>& ppair = ppairs[i];
                  vector<float> embedding = matcher.embedding();

                  if (i && ppair.joint <= prune && ppair.joint < ppairs[i-1].joint)
                    break;

                  if (topN && ++ctr > topN && ppair.indoc.find(docid) == ppair.indoc.end())
                    {
                      skipped_some = true;
                      continue;
                    }
                  if (skipped_some) 
                    {
                      cout << string(17,' ') << "..." << endl;
                      skipped_some = false;
                    }

                  if (topN && ctr > topN) break;

                  
                  // cout << string(80,'-') << endl;
                  // cout << line << endl;
                  // if (refline.size()) cout << refline << endl;
                  // cout << string(80,'-') << endl;
                  
                  // cout << m.str(B->V1.get()) << " (" 
                  //      << s.stats()->trg.size() << " entries; " 
                  //      << indoccnt << "/" << s.stats()->good 
                  //      << " samples in domain; " << num_occurrences
                  //      << " occ.)" << endl;
                  
                  // if (ppair.joint * 100 < ppair.good1) break;
                  ppair.good2 = ppair.raw2 * float(ppair.good1)/ppair.raw1;
                  ppair.good2 = max(ppair.good2, ppair.joint);

#if 1
                  vector<id_type> src_phrase, trg_phrase;
                  B->T1->pid2vec(ppair.p1, src_phrase);
                  B->T2->pid2vec(ppair.p2, trg_phrase);

                  float score = 0;
                  vector<float> v1(matcher.embedding().size(),0);
                  vector<float> v2(v1);
                  vector<bool> check(trg_phrase.size(), false);
                  for (size_t i = 1; i < ppair.aln.size(); i += 2)
                    check[ppair.aln[i]] = true;
                  for (size_t i = 0; i < trg_phrase.size(); ++i)
                    {
                      uint32_t trow = LSA.row2((*B->V2)[trg_phrase[i]]);
                      v1 += LSA.T()[trow];
                      if (check[i]) v2 += LSA.T()[trow];
                      score += log((0.5 + LSA.term_doc_sim(trow, matcher.embedding()))/2);
                    }
                  cout << "\t" 
                       << (fmt % ppair.joint % ppair.good1 % ppair.good2
                           % B->T2->pid2str(B->V2.get(),ppair.p2)
                           % (float(ppair.joint)/ppair.good1)
                           % (float(ppair.joint)/ppair.good2)
                           )
                       << " " << exp(score)
                       << " " << exp(score/trg_phrase.size())
                       << " " << (0.5 + LSA.term_doc_sim(v1, matcher.embedding()))/2
                       << " " << (0.5 + LSA.term_doc_sim(v2, matcher.embedding()))/2
                       << "\n";
                  // print_evidence_list(*B, s.stats()->indoc, ppair);
                  cout << endl;

                  // for (size_t i = 1; i < ppair.aln.size(); i += 2)
                  //   cout << int(ppair.aln[i-1]) << "-"
                  //        << int(ppair.aln[i]) << " ";
                  // cout << endl;


                  // vector<vector<float> >
                  //   E(trg_phrase.size(), matcher.embedding());
                  
                  // for (size_t i = 1; i < ppair.aln.size(); i += 2)
                  //   {
                  //     uint32_t tidx = ppair.aln[i];
                  //     id_type  swrd = src_phrase[ppair.aln[i-1]];
                  //     uint32_t srow = LSA.row1((*B->V1)[swrd]);
                  //     if (srow <= LSA.last_L1_ID())
                  //       E[tidx] -= LSA.T()[srow];
                  //   }
                  // float score = 0;
                  // for (size_t i = 0; i < trg_phrase.size(); ++i)
                  //   {
                  //     uint32_t trow = LSA.row2((*B->V2)[trg_phrase[i]]);
                  //     float score2 = LSA.term_doc_sim(trow, matcher.embedding());
                  //     // float score1 = LSA.term_doc_sim(trow, E[i]);
                  //     // printf("          %+5.3f %+5.3f %s %s\n", score1, score2, 
                  //     //        (*B->V2)[trg_phrase[i]], score1 < 0 ? "BOO" : "");
                  //   }
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
    ("prune,M", po::value<size_t>(&prune)->default_value(1),
     "pruning threshold")
    ("sample,N", po::value<size_t>(&sample_size)->default_value(1000),
     "sample size")
    ("domain,D", po::value<string>(&domain_name),
     "domain name (when reading from stdin)")
    ("reference,r", po::value<string>(&reference_file),
     "reference file")
    ("dalsa", po::value<string>(&dalsa_path),
     "path to LSA models")
    ("dalsa-cols", po::value<size_t>(&dalsa_cols),
     "number of columns in the term-document matrix prior to LSA")
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
