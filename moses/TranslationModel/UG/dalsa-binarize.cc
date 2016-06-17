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

using namespace std;
using namespace Eigen;
using Eigen::MatrixXd;
namespace po=boost::program_options;

void interpret_args(int ac, char* av[]);

string bname, L1, L2, umatrix, smatrix;

// void 
// dump_term_vectors(MatrixXd const& T, MatrixXd const& Srt,
// 		  id_type const  start, id_type const stop,
// 		  string const& fname)
// {
//   ofstream out(fname.c_str());
//   uint64_t num_rows = stop-start;
//   uint32_t num_cols = T.cols();
//   out.write(reinterpret_cast<char*>(&num_rows),8);
//   out.write(reinterpret_cast<char*>(&num_cols),4);
//   for (id_type id = start; id < stop; ++id)
//     {
//       MatrixXd t = T.row(m->second) * Srt;
//       float tnorm = sqrt(t.row(0).dot(t.row(0)));
//       for (size_t c = 0; c < t.cols(); ++c)
// 	{
// 	  float val = t(0,c) / tnorm;
// 	  out.write(reinterpret_cast<char*>(&val),sizeof(float));
// 	}
//     }
// }

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
  boost::iostreams::mapped_file_source idf1(bname+L1+".idf");
  boost::iostreams::mapped_file_source idf2(bname+L2+".idf");

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

  dump_term_vectors(idf1.size() / sizeof(float), bname+L1+".lsi");
  dump_term_vectors(idf2.size() / sizeof(float), bname+L2+".lsi");
  
  // vector<string> row2word;
  // vector<float>  idf;
  // map<string,size_t> L1word2row;
  // map<string,size_t> L2word2row;
  // ifstream termdocmatrix(argv[1]);
  // string line;
  // size_t numdocs = 0;
  // size_t lastid = 0;
  // map<string,size_t>* word2row = &L1word2row;
  // float f;
  // while (getline(termdocmatrix,line))
  //   {
  //     size_t id; string term; float idfval, dummy;
  //     istringstream buf(line);
  //     buf >> id >> term >> idfval;
  //     if (id < lastid) word2row = &L2word2row;
  //     if (id != word2row->size())
  // 	{
  // 	  cerr << "FATAL ERROR: Gap in id sequence!" << endl;
  // 	  exit(1);
  // 	}
  //     (*word2row)[term] = row2word.size();
  //     row2word.push_back(term);
  //     idf.push_back(idfval);
  //     lastid = id;
  //     if (row2word.size() == 1) while (buf >> dummy) ++numdocs;
  //   }

  // ifstream eigenvalues((string(argv[2]) + ".S").c_str());
  // vector<float> eigenvals; 
  // while (eigenvalues >> f) eigenvals.push_back(f);
  // uint32_t num_dimensions = eigenvals.size();

  // MatrixXd S(num_dimensions,num_dimensions);
  // for (size_t i = 0; i < num_dimensions; ++i) 
  //   S(i,i) = eigenvals[i];
  // MatrixXd Srt = S.sqrt();
  
  // MatrixXd T(row2word.size(),num_dimensions);
  // ifstream U((string(argv[2])+".U").c_str());
  // for (size_t r = 0; r < row2word.size(); ++r)
  //   {
  //     getline(U,line);
  //     istringstream buf(line);
  //     for (size_t c = 0; c < num_dimensions; ++c) 
  // 	{
  // 	  buf >> f;
  // 	  T(r,c) = f;
  // 	}
  //   }

  // dump_term_vectors(T, Srt, 0, L1word2row.size(), string(argv[3])+".tv1");
  // dump_term_vectors(T, Srt, L1word2row.size(), row2word.size(), string(argv[3])+".tv2");

  // ofstream idf1((string(argv[3])+".idf1").c_str());
  // idf1.write(&idf[0],L1word2row.size() * sizeof(float));
  // idf1.close();

  // ofstream idf2((string(argv[3])+".idf2").c_str());
  // idf2.write(&idf[L1word2row.size()], L2word2row.size() * sizeof(float));
  // idf2.close();
  
  // ofstream eig((string(argv[3])+".S").c_str());
  
  
  


  // MatrixXd TS = T * S.inverse();
  // cout << TS.rows() << " rows " << TS.cols() << " " << endl;
  
  // string word;
  // MatrixXd d = MatrixXd::Zero(row2word.size(),1);
  // while (cin >> word) 
  //   {
  //     map<string,size_t>::iterator m = L1word2row.find(word);
  //     if (m != L1word2row.end()) ++d(m->second,0);
  //   }
  
  // for (size_t i = 0; i < d.rows(); ++i)
  //   if (d(i,0)) d(i,0) = (1 + log(d(i,0))) * idf[i];
  // d = d.transpose() * TS;
  
  
  // MatrixXd dsrt = d * Srt;
  // float dd = dsrt.row(0) * dsrt.row(0).transpose();

  // for (map<string,size_t>::iterator m = L1word2row.begin(); m != L1word2row.end(); ++m)
  //   {
      
  //     MatrixXd tsrt = T.row(m->second) * Srt;
  //     float tt = tsrt.row(0) * tsrt.row(0).transpose();
  //     float x = (tsrt.row(0) * dsrt.row(0).transpose()); x /= (sqrt(tt) * sqrt(dd));
  //     cout << x << " " << m->first << endl;
  //   }

  // for (map<string,size_t>::iterator m = L2word2row.begin(); m != L2word2row.end(); ++m)
  //   {
      
  //     MatrixXd tsrt = T.row(m->second) * Srt;
  //     float tt = tsrt.row(0) * tsrt.row(0).transpose();
  //     float x = (tsrt.row(0) * dsrt.row(0).transpose()); x /= (sqrt(tt) * sqrt(dd)); 
  //     cout << x << " " << m->first << endl;
  //   }
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
