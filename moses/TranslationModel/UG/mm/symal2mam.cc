// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// program to convert GIZA-style alignments into memory-mapped format
// (c) 2010 Ulrich Germann

// Reads from stdin a file with alternating lines: sentence lengths and symal output.
// We need the sentence lenghts for sanity checks, because GIZA alignment might skip
// sentences. If --skip, we skip such sentence pairs, otherwise, we leave the word
// alignment matrix blank.

#include "ug_mm_ttrack.h"
#include "ug_deptree.h"
#include "tpt_tokenindex.h"
#include "tpt_pickler.h"
#include "moses/TranslationModel/UG/generic/program_options/ug_get_options.h"
#include "moses/TranslationModel/UG/generic/file_io/ug_stream.h"

#include <iostream>
#include <string>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/scoped_ptr.hpp>

#include "util/exception.hh"
// #include "headers-base/util/check.hh"

// NOTE TO SELF:
/* Program to filter out sentences that GIZA will skip or truncate,
 * i.e. sentences longer than 100 words or sentence pairs with a length
 */

using namespace std;
using namespace ugdiss;
using namespace sapt;

ofstream t1out,t2out,mam;
int len1=0,len2=0;
size_t lineCtr=0,sid=0;
bool conll=false;
bool skip=false;
bool debug=false;
TokenIndex V1;

string mtt1name,mtt2name,o1name,o2name,mamname,cfgFile;
string dataFormat,A3filename;
void
interpret_args(int ac, char* av[])
{
  namespace po=boost::program_options;
  po::variables_map vm;
  po::options_description o("Options");
  po::options_description h("Hidden Options");
  po::positional_options_description a;

  o.add_options()
    ("help,h",    "print this message")
    ("cfg,f", po::value<string>(&cfgFile),"config file")
    ("a3",    po::value<string>(&A3filename), "name of A3 file (for sanity checks)")
    ("o1",    po::value<string>(&o1name), "name of output file for track 1")
    ("o2",    po::value<string>(&o2name), "name of output file for track 2")
    ("skip", "skip sentence pairs without word alignment (requires --o1 and --o2)")
    ("debug,d", "debug mode")
    ("t1",    po::value<string>(&mtt1name), "file name of L1 mapped token track")
    ("t2",    po::value<string>(&mtt2name), "file name of L2 mapped token track")
    ("format,F", po::value<string>(&dataFormat)->default_value("plain"), "data format (plain or conll)")
    ;

  h.add_options()
    ("mamname", po::value<string>(&mamname), "name of output file for mam")
    ;
  a.add("mamname",1);

  get_options(ac,av,h.add(o),a,vm,"cfg");

  skip  = vm.count("skip");
  debug = vm.count("debug");
  if (vm.count("help") || mamname.empty())
    {
      cout << "usage:\n"
           << "\t\n"
           << "\t ... | " << av[0]
	   << " <.mam file> \n" << endl;
      cout << o << endl;
      cout << "If an A3 file is given (as produced by (m)giza), symal2mam performs\n"
	   << "a sanity check to make sure that sentence lengths match." << endl;
      exit(0);
    }
  conll = dataFormat == "conll";
  if (!conll and dataFormat != "plain")
    {
      cerr << "format must be 'conll' or 'plain'" << endl;
      exit(1);
    }
  if (skip && (o1name.empty() || o2name.empty()))
    {
      cerr << "--skip requires --o1 and --o2" << endl;
      exit(1);
    }
}

template<typename track_t>
void
copySentence(track_t const& T, size_t sid, ostream& dest)
{
  char const* a = reinterpret_cast<char const*>(T.sntStart(sid));
  char const* z = reinterpret_cast<char const*>(T.sntEnd(sid));
  dest.write(a,z-a);
}

size_t
procSymalLine(string const& line, ostream& out)
{
  ushort a,b; char dash;
  istringstream buf(line);
  while (buf>>a>>dash>>b)
    {
      if (debug && ((len1 && a >= len1) || (len2 && b >= len2)))
        {
          cerr << a << "-" << b << " " << len1 << "/" << len2 << endl;
        }
      assert(len1 == 0 || a<len1);
      assert(len2 == 0 || b<len2);
      tpt::binwrite(out,a);
      tpt::binwrite(out,b);
    }
  return out.tellp();
}

void finiMAM(ofstream& out, vector<id_type>& idx, id_type numTok)
{
  id_type offset = sizeof(filepos_type)+2*sizeof(id_type);
  filepos_type idxStart = out.tellp();
  for (vector<id_type>::iterator i = idx.begin(); i != idx.end(); ++i)
    tpt::numwrite(out,*i-offset);
  out.seekp(0);
  tpt::numwrite(out,idxStart);
  tpt::numwrite(out,id_type(idx.size()-1));
  tpt::numwrite(out,numTok);
  out.close();
}

void
finalize(ofstream& out, vector<id_type> const& idx, id_type tokenCount)
{
  id_type       idxSize = idx.size();
  filepos_type idxStart = out.tellp();
  for (size_t i = 0; i < idx.size(); ++i)
    tpt::numwrite(out,idx[i]);
  out.seekp(0);
  tpt::numwrite(out,idxStart);
  tpt::numwrite(out,idxSize-1);
  tpt::numwrite(out,tokenCount);
  out.close();
}

bool getCheckValues(istream& in, int& check1, int& check2)
{
  if (A3filename.empty()) return true;
  string line; string w;
  getline(in,line);
  size_t p1 = line.find("source length ") + 14;
  if (p1 >= line.size()) return false;
  size_t p2 = line.find("target length ",p1);
  if (p2 >= line.size()) return false;
  // cout << line << endl;
  // cout << line.substr(p1,p2-p1) << endl;
  check1 = atoi(line.substr(p1,p2-p1).c_str());
  p1 = p2+14;
  p2 = line.find("alignment ",p1);
  if (p2 >= line.size()) return false;
  check2 = atoi(line.substr(p1,p2-p1).c_str());
  getline(in,line);
  getline(in,line);
  return true;
}

void
go()
{
  size_t ctr=0;
  vector<id_type> idxm;
  idxm.reserve(10000000);
  idxm.push_back(mam.tellp());
  string line;
  while(getline(cin,line))
    {
      idxm.push_back(procSymalLine(line,mam));
      if (debug && ++ctr%100000==0)
	cerr << ctr/1000 << "K lines processed" << endl;
    }
  finiMAM(mam,idxm,0);
  cout << idxm.size() << endl;
}

template<typename TKN>
void
go(string t1name, string t2name, string A3filename)
{
  typedef mmTtrack<TKN> track_t;
  track_t T1(t1name),T2(t2name);
  boost::iostreams::filtering_istream A3file; 
  open_input_stream(A3filename, A3file);

  string line; int check1=-1,check2=-1;
  vector<id_type> idx1(1,0),idx2(1,0),idxm(1, mam.tellp());
  size_t tokenCount1=0,tokenCount2=0;
  size_t skipCtr=0,lineCtr=0;
  if (!getCheckValues(A3file, check1, check2))
    UTIL_THROW(util::Exception, "Mismatch in input files!");

  for (sid = 0; sid < T1.size(); ++sid)
    {
      len1 = T1.sntLen(sid);
      len2 = T2.sntLen(sid);
      if (debug)
        cerr << "[" << lineCtr << "] "
             << len1 << " (" << check1 << ") / "
             << len2 << " (" << check2 << ")" << endl;
      if ((check1 >=0 && check1!=len1) ||
	  (check2 >=0 && check2!=len2))
        {
          if (skip)
            {
              cerr << "[" << ++skipCtr << "] skipping "
                   << check1 << "/" << check2 << " vs. "
                   << len1 << "/" << len2
                   << " at line " << lineCtr << endl;
            }
          else
            {
              idxm.push_back(mam.tellp());
            }
          if (len1 > 100 || len2 > 100)
            {
              getline(cin,line);
              getCheckValues(A3file,check1,check2);
              lineCtr++;
            }
          continue;
        }
      if (skip)
        {
          idx1.push_back(tokenCount1 += len1);
          copySentence(T1,sid,t1out);
          idx2.push_back(tokenCount2 += len2);
          copySentence(T2,sid,t2out);
        }

      if (!getline(cin,line))
	UTIL_THROW(util::Exception, "Too few lines in symal input!");

      lineCtr++;
      idxm.push_back(procSymalLine(line,mam));
      if (debug) cerr << "[" << lineCtr << "] "
                      << check1 << " (" << len1 <<") "
                      << check2 << " (" << len2 <<") "
                      << line << endl;
      getCheckValues(A3file,check1,check2);
    }
  if (skip)
    {
      finalize(t1out,idx1,tokenCount1);
      finalize(t2out,idx2,tokenCount2);
    }
  finiMAM(mam,idxm,0);
  cout << idxm.size() << endl;
}

void
initialize(ofstream& out, string const& fname)
{
  out.open(fname.c_str());
  tpt::numwrite(out,filepos_type(0)); // place holder for index start
  tpt::numwrite(out,id_type(0));      // place holder for index size
  tpt::numwrite(out,id_type(0));      // place holder for token count
}

int main(int argc, char* argv[])
{
  interpret_args(argc,argv);
  if (skip)
    {
      initialize(t1out,o1name);
      initialize(t2out,o2name);
    }
  initialize(mam,mamname);
  if (A3filename.size() == 0)
    go();
  else if (conll)
    go<Conll_Record>(mtt1name,mtt2name,A3filename);
  else
    go<id_type>(mtt1name,mtt2name,A3filename);
}
