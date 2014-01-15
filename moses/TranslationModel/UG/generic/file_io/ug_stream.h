// -*- c++ -*-
// (c) 2006,2007,2008 Ulrich Germann
// makes opening files a little more convenient

#ifndef __UG_STREAM_HH
#define __UG_STREAM_HH
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/categories.hpp> // input_filter_tag
#include <boost/iostreams/operations.hpp> // get, WOULD_BLOCK
#include <boost/iostreams/copy.hpp>       // get, WOULD_BLOCK
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <iostream>
#include <fstream>
#include <string>

namespace ugdiss
{
using namespace std;
using namespace boost::iostreams;

/** open input file that is possibly compressed
 *  decompression filters are automatically added based on the file name
 *  gzip for .gz; bzip2 for bz2. 
 */
filtering_istream* open_input_stream(string fname);
void open_input_stream(string fname, filtering_istream& in);
// filtering_streambuf<input>* open_input_stream(string fname);

filtering_ostream* open_output_stream(string fname);
void open_output_stream(string fname, filtering_ostream& in);


}
#endif
