//-*- c++ -*-
#include "ug_splice_arglist.h"
#include "moses/Util.h"
#include "util/exception.hh"
#include <boost/foreach.hpp>

namespace Moses {

  void
  filter_arguments(int const argc_in, char const* const* const argv_in,
		   int & argc_moses, char*** argv_moses,
		   int & argc_other, char*** argv_other,
		   std::vector<std::pair<std::string,int> > const& filter)
  {
    *argv_moses = new char*[argc_in];
    *argv_other = new char*[argc_in];
    (*argv_moses)[0] = new char[strlen(argv_in[0])+1];
    strcpy((*argv_moses)[0], argv_in[0]);
    argc_moses = 1;
    argc_other = 0;
    typedef std::pair<std::string,int> option;
    int i = 1;
    while (i < argc_in)
      {
	BOOST_FOREACH(option const& o, filter)
	  {
	    if (o.first == argv_in[i])
	      {
		(*argv_other)[argc_other] = new char[strlen(argv_in[i])+1];
		strcpy((*argv_other)[argc_other++],argv_in[i]);
		for (int k = 0; k < o.second; ++k)
		{
		  UTIL_THROW_IF2(++i >= argc_in || argv_in[i][0] == '-',
				 "[" << HERE << "] Missing argument for "
				 << "parameter " << o.first << "!");
		  (*argv_other)[argc_other] = new char[strlen(argv_in[i])+1];
		  strcpy((*argv_other)[argc_other++],argv_in[i]);
		}
		if (++i >= argc_in) break;
	      }
	  }
	if (i >= argc_in) break;
	(*argv_moses)[argc_moses] = new char[strlen(argv_in[i])+1];
	strcpy((*argv_moses)[argc_moses++], argv_in[i++]);
      }
  }

} // namespace Moses


