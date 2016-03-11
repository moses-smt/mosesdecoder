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

#include "mm/tpt_tokenindex.h"

using namespace std;
using namespace Eigen;
using Eigen::MatrixXd;
namespace po=boost::program_options;

void interpret_args(int ac, char* av[]);

string bname, L1, L2, umatrix, smatrix;
sapt::TokenIndex V1,V2;

ifstream U;
vector<float> Srt; 

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


int main(int argc, char* argv[])
{
  interpret_args(argc, argv);
  V1.open(bname+L1+".tdx");
  V2.open(bname+L2+".tdx");
  
  float f; string line;
  ifstream S_in(smatrix.c_str()); 
  ofstream S_out((bname+"lsi.S").c_str());
  while (getline(S_in,line)) 
    //while (S_in >> f)
    {
      cerr << line << endl;
      f = atof(line.c_str());
      Srt.push_back(sqrt(f));
      S_out << f << endl;
    }
  
  U.open(umatrix.c_str());
  
  dump_term_vectors(V1.ksize(), bname + L1 + ".term-vectors");
  dump_term_vectors(V2.ksize(), bname + L2 + ".term-vectors");
  
}


void
interpret_args(int ac, char* av[])
{
  po::variables_map vm;
  po::options_description o("Options");
  o.add_options()
    ("help,h",  "print this message")
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
