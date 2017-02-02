#include "ug_load_primer.h"
#include <boost/interprocess/mapped_region.hpp>
#include <boost/thread.hpp>

namespace Moses
{
  FastLoader::
  FastLoader(boost::iostreams::mapped_file_source const& f)
    : file(f) {}


  void
  FastLoader::
  operator()() const
  {
    size_t const pagesize = boost::interprocess::mapped_region::get_page_size();
    char const* stop = file.data() + file.size();
    int dummy=0;
    for (char const* x = file.data(); x < stop; x += pagesize) dummy += *x;
  }

  void prime(boost::iostreams::mapped_file_source const& f)
  {
    boost::thread foo(FastLoader(f));
    // foo.detach();
  }

}
