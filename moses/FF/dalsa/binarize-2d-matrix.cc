// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdint.h>
#include <cassert>
#include <stdlib.h>
#include "ug_mm_dense_table.h"
#include "moses/TranslationModel/UG/generic/threading/ug_thread_pool.h"
#include <boost/scoped_ptr.hpp>
#include <boost/format.hpp>

using namespace std;

ug::mmTable::mmTable<float,2> T;
size_t num_cols, num_rows;

struct lineprocessor
{
  string line;
  size_t row;
  
  lineprocessor(size_t r) : row(r) {}
  
  void 
  operator()()
  {
    istringstream buf(line);
    size_t c = 0;
    while (buf >> T[row][c]) ++c;
    if (c != num_cols) 
      {
        string fmt = "Mismatch in number of values: expected %d but got %d.";
        throw (boost::format(fmt.c_str()) % num_cols % c).str().c_str();
      }
  }
};

struct converter
{
  char *start, *stop;
  size_t row;

  converter(size_t r, char* _start, char* _stop) 
  : row(r), start(_start), stop(_stop) {}

  void 
  operator()() 
  {
    char *q = stop, *p = start;
    size_t c;
    for (c = 0; c < num_cols; ++c, p = q)
      {
        T[row][c] = strtof(p,&q);
        if (q == p) break;
      }
    if (c != num_cols) 
      {
        cerr << num_cols << " columns expected but " << c 
             << " encountered in row " << row << endl;
        throw "Column count error.";
      }
  }
  
};

int main(int argc, char* argv[]) 
{
  boost::scoped_ptr<ug::ThreadPool> threadpool;
  threadpool.reset(new ug::ThreadPool(boost::thread::hardware_concurrency()));
  string line; float f;
  
  uint64_t dims[2];
  string rowspec = argv[3];
  string colspec = argv[4];
  sscanf(rowspec.c_str(), "%zu", &dims[0]);
  sscanf(colspec.c_str(), "%zu", &dims[1]);

  num_rows = dims[0];
  num_cols = dims[1];

  cerr << num_rows << " rows of " << num_cols << " columns." << endl;

  boost::iostreams::mapped_file_params param;
  param.path = argv[1];
  param.flags = boost::iostreams::mapped_file::priv;

  boost::iostreams::mapped_file data(param);

  char* stop = data.data() + data.size() - 1;
  T.create(argv[2], dims);
  size_t row = 0;
  for (char *q, *p = data.data(); p < stop; p = q + 1, ++row)
    {

      if (row == num_rows) break;

      if (row % 100000 == 0) cerr << row/1000 << "K"; 
      else if (row % 10000 == 0) cerr << ".";

      for (q = p+1; q < stop && *q != '\n'; ++q);
      converter job(row, p, q);
      threadpool->add(job);
    }
  cerr << row << "/" << num_rows << " rows processed" << endl;
  threadpool.reset();
  T.close();
}
