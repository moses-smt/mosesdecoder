// -*- c++ -*-
// (c) 2006,2007,2008 Ulrich Germann
// makes opening files a little more convenient

#include "ug_stream.h"

namespace ugdiss
{
  using namespace std;
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
    if (fname.size()>3 && fname.substr(fname.size()-3,3)==".gz")
      {
	in.push(gzip_decompressor());
      }
    else if (fname.size() > 4 && fname.substr(fname.size()-4,4)==".bz2")
      {
	in.push(bzip2_decompressor());
      }
    in.push(file_source(fname.c_str()));
  }

  void 
  open_output_stream(string fname, filtering_ostream& out)
  {
    if ((fname.size() > 3 && fname.substr(fname.size()-3,3)==".gz") ||
	(fname.size() > 4 && fname.substr(fname.size()-4,4)==".gz_"))
      {
	out.push(gzip_compressor());
      }
    else if ((fname.size() > 4 && fname.substr(fname.size()-4,4)==".bz2") ||
	     (fname.size() > 5 && fname.substr(fname.size()-5,5)==".bz2_"))
      {
	out.push(bzip2_compressor());
      }
    out.push(file_sink(fname.c_str()));
  }
}
