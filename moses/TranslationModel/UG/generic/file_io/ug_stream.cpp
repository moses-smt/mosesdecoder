// -*- c++ -*-
// (c) 2006,2007,2008 Ulrich Germann
// makes opening files a little more convenient

#include <boost/algorithm/string/predicate.hpp>
#include "ug_stream.h"

namespace ugdiss
{
  using namespace std;
  using namespace boost::algorithm;
  using namespace boost::iostreams;

  filtering_istream*
  open_input_stream(string fname)
  {
    filtering_istream* ret = new filtering_istream();
    open_input_stream(fname,*ret);
    return ret;
  }

  filtering_ostream*
  open_output_stream(string fname)
  {
    filtering_ostream* ret = new filtering_ostream();
    open_output_stream(fname,*ret);
    return ret;
  }

  void
  open_input_stream(string fname, filtering_istream& in)
  {
    if (ends_with(fname, ".gz"))
      {
	in.push(gzip_decompressor());
      }
    else if (ends_with(fname, "bz2"))
      {
	in.push(bzip2_decompressor());
      }
    in.push(file_source(fname.c_str()));
  }

  void
  open_output_stream(string fname, filtering_ostream& out)
  {
    if (ends_with(fname, ".gz") || ends_with(fname, ".gz_"))
      {
	out.push(gzip_compressor());
      }
    else if (ends_with(fname, ".bz2") || ends_with(fname, ".bz2_"))
      {
	out.push(bzip2_compressor());
      }
    out.push(file_sink(fname.c_str()));
  }
}
