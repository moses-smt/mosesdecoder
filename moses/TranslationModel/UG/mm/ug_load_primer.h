//-*- c++ -*-
#pragma once
#include <boost/iostreams/device/mapped_file.hpp>
// 
namespace Moses
{
  class FastLoader
  {
    boost::iostreams::mapped_file_source const& file;
  public:
    FastLoader(boost::iostreams::mapped_file_source const& f);
    void operator()() const;
  };

  void prime(boost::iostreams::mapped_file_source const& f);

    
};
