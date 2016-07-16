// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdint.h>
#include <cassert>
#include <cmath>
#include <boost/iostreams/device/mapped_file.hpp>
#include "ug_mm_dense_table.h"
#include <boost/format.hpp>

using namespace std;

boost::iostreams::mapped_file infile;
boost::iostreams::mapped_file outfile;

size_t num_vectors, num_dimensions;

int main(int argc, char* argv[]) 
{
  ug::mmTable::mmTable<float,2> T;
  T.open(argv[1]);
  // cerr << T.size(0) << " rows, " << T.size(1) << " columns" << endl;
  for (size_t r = 0; r < T.size(0); ++r)
    {
      cout << T[r][0];
      for (size_t c = 1; c < T.size(1); ++c)
        cout << " " << T[r][c];
      cout << endl;
    }
  T.close();
}
