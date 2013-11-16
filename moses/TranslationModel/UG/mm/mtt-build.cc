// -*- c++ -*-
// Converts a corpus in text format (plain text, one centence per line) or
// conll format or treetagger output format (which one is automatically
// recognized based on the number of fields per line) into memory-mapped
// format. (c) 2007-2013 Ulrich Germann

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

#include <sys/types.h>
#include <sys/wait.h>

#include "ug_conll_record.h"
#include "tpt_tokenindex.h"
#include "ug_mm_ttrack.h"
#include "tpt_pickler.h"
#include "ug_deptree.h"
#include "moses/generic/sorting/VectorIndexSorter.h"
#include "ug_im_tsa.h"

using namespace std;
using namespace ugdiss;
using namespace Moses;
namespace po=boost::program_options;

int with_pfas;
int with_dcas;
int with_sfas;

bool incremental = false; // build / grow vocabs automatically
bool is_conll    = false; // text or conll format?
bool quiet       = false; // no progress reporting

string vocabBase; // base name for existing vocabs that should be used
string baseName;  // base name for all files
string tmpFile, mttFile;   /* name of temporary / actual track file 
			    * (.mtt for Conll format, .mct for plain text) 
			    */
string UNK;

TokenIndex SF; // surface form
TokenIndex LM; // lemma
TokenIndex PS; // part of speech
TokenIndex DT; // dependency type

void interpret_args(int ac, char* av[]);

inline uchar rangeCheck(int p, int limit) { return p < limit ? p : 1; }

id_type 
get_id(TokenIndex const& T, string const& w)
{
  id_type ret = T[w];
  if (ret == 1 && w != UNK)
    {
      cerr << "Warning! Unkown vocabulary item '" << w << "', but "
	   << "incremental mode (-i) is not set." << endl;
      assert(0);
    }
  return ret;
}

void 
open_vocab(TokenIndex& T, string fname)
{
  if (!access(fname.c_str(), F_OK)) 
    { 
      T.open(fname,UNK); 
      assert(T[UNK] == 1); 
    }
  else T.setUnkLabel(UNK);
  if (incremental) T.setDynamic(true);
  assert(T["NULL"] == 0); 
  assert(T[UNK]  == 1);
}

void 
ini_cnt_vec(TokenIndex const& T, vector<pair<string,size_t> > & v)
{
  v.resize(T.totalVocabSize());
  for (size_t i = 0; i < T.totalVocabSize(); ++i)
    {
      v[i].first = T[i];
      v[i].second = 0;
    }
}

void
write_tokenindex(string fname, TokenIndex& T, vector<id_type> const& n2o)
{
  if (!quiet) cerr << "Writing " << fname << endl;
  vector<id_type> o2n(n2o.size());
  for (id_type i = 0; i < n2o.size(); ++i) o2n[n2o[i]] = i;
  vector<pair<string,uint32_t> > v(n2o.size());
  for (id_type i = 0; i < n2o.size(); ++i)
    {
      v[i].first  = T[n2o[i]];
      v[i].second = i;
    }
  T.close();
  sort(v.begin(),v.end());
  write_tokenindex_to_disk(v, fname, UNK);
}

void init(int argc, char* argv[])
{
  interpret_args(argc,argv);
  if (is_conll)
    {
      open_vocab(SF, vocabBase+".tdx.sfo"); // surface form
      open_vocab(LM, vocabBase+".tdx.lem"); // lemma
      open_vocab(PS, vocabBase+".tdx.pos"); // part-of-speech
      open_vocab(DT, vocabBase+".tdx.drl"); // dependency type
    }
  else open_vocab(SF, vocabBase+".tdx"); // surface form
}

void fill_rec(Conll_Record& rec, vector<string> const& w)
{
  if (w.size() == 3) // treetagger output
    {
      rec.sform  =  get_id(SF, w[0]);
      rec.lemma  =  get_id(LM, w[2] == "<UNKNOWN>" ? w[0] : w[2]);
      rec.majpos =  rangeCheck(get_id(PS, w[1]), 256);
      rec.minpos =  rangeCheck(get_id(PS, w[1]), 256);
      rec.dtype  =  0;
      rec.parent = -1;
    }
  else if (w.size() >= 8) // CONLL format
    {
      int id  = atoi(w[0].c_str());
      int gov = atoi(w[6].c_str()); 
      rec.sform  = get_id(SF, w[1]);
      rec.lemma  = get_id(LM, w[2]);
      rec.majpos = rangeCheck(get_id(PS, w[3]), 256);
      rec.minpos = rangeCheck(get_id(PS, w[4]), 256);
      rec.dtype  = get_id(DT, w[7]);
      rec.parent = gov ? gov - id : 0;
    }
}

void log_progress(size_t ctr)
{
  if (ctr % 100000 == 0)
    {
      if (ctr) cerr << endl;
      cerr << setw(12) << ctr / 1000 << "K sentences processed ";
    }
  else if (ctr % 10000 == 0)
    {
      cerr << "."; 
    }
}


size_t 
process_plain_input(ostream& out, vector<id_type> & s_index)
{
  id_type totalWords = 0;
  string line,w;
  while (getline(cin,line))
    {
      istringstream buf(line);
      if (!quiet) log_progress(s_index.size());
      s_index.push_back(totalWords);
      while (buf>>w) 
	{
	  numwrite(out,get_id(SF,w));
	  ++totalWords;
	}
    }
  s_index.push_back(totalWords);
  return totalWords;
}

size_t 
process_tagged_input(ostream& out, 
		     vector<id_type> & s_index, 
		     vector<id_type> & p_index)
{
  string line;
  Conll_Record rec;
  bool new_sent  = true;
  bool new_par   = true;
  id_type totalWords = 0;
  
  while (getline(cin,line))
    {
      vector<string> w; string f; istringstream buf(line);
      while (buf>>f) w.push_back(f);

      if (w.size() == 0 || (w[0].size() >= 4 && w[0].substr(0,4) == "SID="))
        new_sent = true;

      else if (w.size() == 1 && w[0] == "<P>") 
	new_par = new_sent = true;

      if (w.size() < 3) continue;
      if (!quiet && new_sent) log_progress(s_index.size());
      if (new_sent) { s_index.push_back(totalWords); new_sent = false; }
      if (new_par)  { p_index.push_back(totalWords); new_par  = false; }
      fill_rec(rec,w);
      out.write(reinterpret_cast<char const*>(&rec),sizeof(rec));
      ++totalWords;
    }
  s_index.push_back(totalWords);
  return totalWords;
}

size_t
numberize()
{
  ofstream out(tmpFile.c_str());
  filepos_type startIdx=0;
  id_type idxSize=0,totalWords=0;
  numwrite(out,startIdx);   // place holder, to be filled at the end
  numwrite(out,idxSize);    // place holder, to be filled at the end
  numwrite(out,totalWords); // place holder, to be filled at the end

  vector<id_type> s_index, p_index;

  if(is_conll)
    totalWords = process_tagged_input(out,s_index,p_index);
  else
    totalWords = process_plain_input(out,s_index);

  vector<id_type> const* index = &s_index;
  if (p_index.size() && p_index.back())
    {
      p_index.push_back(totalWords);
      index = &p_index;
    }

  if (!quiet) 
    cerr << endl << "Writing index ... (" << index->size() << " chunks) ";

  startIdx = out.tellp();
  for (size_t i = 0; i < index->size(); i++) numwrite(out,(*index)[i]);
  out.seekp(0);
  idxSize = index->size();
  numwrite(out, startIdx);
  numwrite(out, idxSize - 1);
  numwrite(out, totalWords);
  out.close();
  if (!quiet) cerr << "done" << endl;
  return totalWords;
}

vector<id_type> smap,lmap,pmap,dmap;

void 
invert(vector<id_type> const& from, vector<id_type> & to)
{
  to.resize(from.size());
  for (size_t i = 0 ; i < to.size(); ++i)
    to[from[i]] = i;
}

// sorts new items based on occurrence counts but won't reassign 
// existing token ids
void 
conservative_sort(TokenIndex     const & V, 
		  vector<size_t> const & cnt, 
		  vector<id_type>      & xmap)
{
  xmap.resize(V.totalVocabSize());
  for (size_t i = 0; i < xmap.size(); ++i) xmap[i] = i;
  VectorIndexSorter<size_t,greater<size_t>, id_type> sorter(cnt);
  sort(xmap.begin()+max(id_type(2),V.knownVocabSize()), xmap.end(), sorter);
}

// reassign token ids in the corpus track based on the id map created by
// conservative_sort
void remap()
{
  if (!quiet) cerr << "Remapping ids ... ";
  filepos_type idxOffset;
  id_type totalWords, idxSize;
  boost::iostreams::mapped_file mtt(tmpFile);
  char const* p = mtt.data();
  p = numread(p,idxOffset);
  p = numread(p,idxSize);
  p = numread(p,totalWords);
  if (is_conll)
    {
      vector<size_t> sf(SF.totalVocabSize(), 0);
      vector<size_t> lm(LM.totalVocabSize(), 0);
      vector<size_t> ps(PS.totalVocabSize(), 0);
      vector<size_t> dt(DT.totalVocabSize(), 0);
      Conll_Record* w  = reinterpret_cast<Conll_Record*>(const_cast<char*>(p));
      for (size_t i = 0; i < totalWords; ++i)
	{
	  ++sf.at(w[i].sform);
	  ++lm.at(w[i].lemma);
	  ++ps.at(w[i].majpos);
	  ++ps.at(w[i].minpos);
	  ++dt.at(w[i].dtype);
	}
      conservative_sort(SF,sf,smap);
      conservative_sort(LM,lm,lmap);
      conservative_sort(PS,ps,pmap);
      conservative_sort(DT,dt,dmap);
      vector<id_type> smap_i(smap.size()); invert(smap,smap_i);
      vector<id_type> lmap_i(lmap.size()); invert(lmap,lmap_i);
      vector<id_type> pmap_i(pmap.size()); invert(pmap,pmap_i);
      vector<id_type> dmap_i(dmap.size()); invert(dmap,dmap_i);
      for (size_t i = 0; i < totalWords; ++i)
	{
	  w[i].sform  = smap_i[w[i].sform];
	  w[i].lemma  = lmap_i[w[i].lemma];
	  w[i].majpos = pmap_i[w[i].majpos];
	  w[i].minpos = pmap_i[w[i].minpos];
	  w[i].dtype  = dmap_i[w[i].dtype];
	}
    }
  else
    {
      vector<size_t> sf(SF.totalVocabSize(), 0);
      id_type* w = reinterpret_cast<id_type*>(const_cast<char*>(p));
      for (size_t i = 0; i < totalWords; ++i) ++sf.at(w[i]);
      conservative_sort(SF,sf,smap);
      vector<id_type> smap_i(smap.size()); invert(smap,smap_i);
      for (size_t i = 0; i < totalWords; ++i) w[i] = smap_i[w[i]];
    }
  mtt.close();
  if (!quiet) cerr << "done." << endl;
}

void save_vocabs()
{
  string vbase = baseName;
  if (is_conll)
    {
      if (SF.totalVocabSize() > SF.knownVocabSize()) 
	write_tokenindex(vbase+".tdx.sfo",SF,smap);
      if (LM.totalVocabSize() > LM.knownVocabSize()) 
	write_tokenindex(vbase+".tdx.lem",LM,lmap);
      if (PS.totalVocabSize() > PS.knownVocabSize()) 
	write_tokenindex(vbase+".tdx.pos",PS,pmap);
      if (DT.totalVocabSize() > DT.knownVocabSize()) 
	write_tokenindex(vbase+".tdx.drl",DT,dmap);
    }
  else if (SF.totalVocabSize() > SF.knownVocabSize()) 
    write_tokenindex(vbase+".tdx",SF,smap);
}

template<typename Token>
size_t 
build_mmTSA(string infile, string outfile)
{
  size_t mypid = fork();
  if(mypid) return mypid;
  mmTtrack<Token> T(infile);
  bdBitset filter;
  filter.resize(T.size(),true);
  imTSA<Token> S(&T,filter,(quiet?NULL:&cerr));
  S.save_as_mm_tsa(outfile);
  exit(0);
}

bool 
build_plaintext_tsas()
{
  typedef L2R_Token<SimpleWordId> L2R;
  typedef R2L_Token<SimpleWordId> R2L;
  size_t c = with_sfas + with_pfas;
  if (with_sfas) build_mmTSA<L2R>(tmpFile, baseName + ".sfa"); 
  if (with_pfas) build_mmTSA<R2L>(tmpFile, baseName + ".pfa"); 
  while (c--) wait(NULL);
  return true;
}

void build_conll_tsas()
{
  string bn  = baseName;
  string mtt = tmpFile;
  size_t c = 3 * (with_sfas + with_pfas + with_dcas);
  if (with_sfas) 
    {
      build_mmTSA<L2R_Token<Conll_Sform> >(mtt,bn+".sfa-sform");
      build_mmTSA<L2R_Token<Conll_Lemma> >(mtt,bn+".sfa-lemma");
      build_mmTSA<L2R_Token<Conll_MinPos> >(mtt,bn+".sfa-minpos");
    }

  if (with_pfas) 
    {
      build_mmTSA<R2L_Token<Conll_Sform> >(mtt,bn+".pfa-sform");
      build_mmTSA<R2L_Token<Conll_Lemma> >(mtt,bn+".pfa-lemma");
      build_mmTSA<R2L_Token<Conll_MinPos> >(mtt,bn+".pfa-minpos");
    }

  if (with_dcas) 
    {
      build_mmTSA<ConllBottomUpToken<Conll_Sform> >(mtt,bn+".dca-sform");  
      build_mmTSA<ConllBottomUpToken<Conll_Lemma> >(mtt,bn+".dca-lemma");  
      build_mmTSA<ConllBottomUpToken<Conll_MinPos> >(mtt,bn+".dca-minpos");
    }
  while (c--) wait(NULL); 
}


int main(int argc, char* argv[])
{
  init(argc,argv);
  numberize();
  if (SF.totalVocabSize() > SF.knownVocabSize() ||
      LM.totalVocabSize() > LM.knownVocabSize() ||
      PS.totalVocabSize() > PS.knownVocabSize() ||
      DT.totalVocabSize() > DT.knownVocabSize())
    {
      remap();
      save_vocabs();
    }
  if (is_conll) build_conll_tsas();
  else          build_plaintext_tsas();
  if (!quiet) cerr << endl;
  rename(tmpFile.c_str(),mttFile.c_str());
}

void 
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()

    ("help,h",  "print this message")

    ("quiet,q", po::bool_switch(&quiet), 
     "don't print progress information")

    ("incremental,i", po::bool_switch(&incremental), 
     "incremental mode; rewrites vocab files!")

    ("vocab-base,v", po::value<string>(&vocabBase),
     "base name of various vocabularies")

    ("output,o", po::value<string>(&baseName),
     "base file name of the resulting file(s)")

    ("sfa,s", po::value<int>(&with_sfas)->default_value(1), 
     "also build suffix arrays")

    ("pfa,p", po::value<int>(&with_pfas)
     ->default_value(0)->implicit_value(1), 
     "also build prefix arrays")

    ("dca,d", po::value<int>(&with_dcas)
     ->default_value(0)->implicit_value(1), 
     "also build dependency chain arrays")

    ("conll,c", po::bool_switch(&is_conll),
     "corpus is in CoNLL format (default: plain text)")

    ("unk,u", po::value<string>(&UNK)->default_value("UNK"),
     "label for unknown tokens")

    // ("map,m", po::value<string>(&vmap), 
    // "map words to word classes for indexing")
    
    ;
  
  po::options_description h("Hidden Options");
  h.add_options()
    ;
  h.add(o);
  po::positional_options_description a;
  a.add("output",1);
  
  po::store(po::command_line_parser(ac,av)
            .options(h)
            .positional(a)
            .run(),vm);
  po::notify(vm);
  if (vm.count("help") || !vm.count("output"))
    {
      cout << "\nusage:\n\t cat <corpus> | " << av[0] 
           << " [options] <output .mtt file>" << endl;
      cout << o << endl;
      exit(0);
    }
  mttFile = baseName + (is_conll ? ".mtt" : ".mct");
  tmpFile = mttFile + "_";
}
