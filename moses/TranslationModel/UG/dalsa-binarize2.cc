// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/program_options.hpp>

#include <unsupported/Eigen/MatrixFunctions>
#include <Eigen/Core>
#include <Eigen/Dense>

#include <map>
#include <vector>
#include <string>

#include "mm/ug_bitext.h"

using namespace std;
using namespace Eigen;
using namespace Moses;
using namespace sapt;
using Eigen::MatrixXd;
namespace po=boost::program_options;

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef mmBitext<Token> bitext_t;

void interpret_args(int ac, char* av[]);

string bname, L1, L2, umatrix, smatrix;
size_t minocc=5;
bool raw;
boost::shared_ptr<bitext_t> B(new bitext_t);

ifstream U;
vector<float> Srt; 

void 
dump_raw_term_vectors(uint64_t const vsize, string const& ofname)
{
  string line; float f;
  uint32_t numdims = Srt.size();
  ofstream out(ofname.c_str());
  out.write(reinterpret_cast<char const*>(&vsize), sizeof(vsize));
  out.write(reinterpret_cast<char const*>(&numdims), sizeof(numdims));
  for (size_t id = 0; id < vsize; ++id)
    {
      getline(U,line);
      istringstream buf(line);
      while (buf >> f)
        out.write(reinterpret_cast<char const*>(&f), sizeof(float));
    }
}

void 
dump_term_vectors(uint64_t const vsize, string const& ofname)
{
  string line; float f;
  ofstream out(ofname.c_str());
  out.write(reinterpret_cast<char const*>(&vsize), sizeof(vsize));
  uint32_t numdims = Srt.size()+1;
  out.write(reinterpret_cast<char const*>(&numdims), sizeof(numdims));
  for (size_t id = 0; id < vsize; ++id)
    {
      getline(U,line);
      istringstream buf(line);
      vector<float> tmp(Srt.size());
      float total = 0;
      for (size_t i = 0; buf >> f; ++i)
        total += pow(tmp[i] = f * Srt[i], 2);
      total = sqrt(total);
      for (size_t i = 0; i < tmp.size(); ++i)
        out.write(reinterpret_cast<char const*>(&(total 
                                                  ? tmp[i] /= total 
                                                  : tmp[i])), 
                  sizeof(float));
      out.write(reinterpret_cast<char const*>(&total), sizeof(float));
    }
}

typedef sapt::L2R_Token<sapt::SimpleWordId> Token;
typedef mmBitext<Token> bitext_t;

size_t 
vsize(TSA<Token> const* idx)
{
 bitext_t::iter m(idx);
  size_t vsize = 0;
  m.down(); 
  while (vsize <= 2 || m.ca() >= minocc)
    {
      ++vsize;
      if (!m.over()) break;
    }
  return vsize;
}

int main(int argc, char* argv[])
{
  interpret_args(argc, argv);
  B->open(bname, L1, L2);
  size_t vsize1 = vsize(B->I1.get());
  size_t vsize2 = vsize(B->I2.get());

  cerr << vsize1 << endl;
  cerr << vsize2 << endl;

  float f; string line;
  ifstream S_in(smatrix.c_str()); 
  ofstream S_out((bname+"lsi.S").c_str());
  while (getline(S_in,line)) 
    {
      f = atof(line.c_str());
      Srt.push_back(sqrt(f));
      S_out << f << endl;
    }
  
  U.open(umatrix.c_str());
  if (raw)
    {
      dump_term_vectors(vsize1, bname + L1 + ".T");
      dump_term_vectors(vsize2, bname + L2 + ".T");
    }
  else
    {
      dump_term_vectors(vsize1, bname + L1 + ".term-vectors");
      dump_term_vectors(vsize2, bname + L2 + ".term-vectors");
    }
}

void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
    ("min-occurrences,m",po::value<size_t>(&minocc)->default_value(5),
     "occurrence threshold for terms")
    ("raw,r",po::bool_switch(&raw),"use raw values")
    ;

  po::options_description h("Hidden Options");
  h.add_options()
    ("bname", po::value<string>(&bname), "base name of corpus")
    ("L1", po::value<string>(&L1), "L1 tag")
    ("L2", po::value<string>(&L2), "L2 tag")
    ("U", po::value<string>(&umatrix), "matrix U")
    ("S", po::value<string>(&smatrix), "matrix S")
    ;

  h.add(o);
  po::positional_options_description a;
  a.add("bname",1);
  a.add("L1",1);
  a.add("L2",1);
  a.add("U",1);
  a.add("S",1);

  po::store(po::command_line_parser(ac,av)
            .options(h)
            .positional(a)
            .run(),vm);
  po::notify(vm);
  if (vm.count("help"))
    {
      std::cout << "\nusage:\n\t" << av[0]
           << " [options] <model file stem> <L1> <L2> <matrix U> <matrix S>" << std::endl;
      std::cout << o << std::endl;
      exit(0);
    }
}
