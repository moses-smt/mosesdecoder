// -*- c++ -*-
// (c) 2006,2007,2008 Ulrich Germann
#ifndef __Pickler
#define __Pickler

#include<iostream>
#include<string>
#include<vector>
#include<map>
#include "tpt_typedefs.h"
#include "num_read_write.h"
#include <cassert>

namespace ugdiss
{
  /// Utility method placed here for lack of a better place
  /// @return the size of file fname.
  uint64_t getFileSize(const std::string& fname);

  /** 
   * The following functions write and read data in a compact binary 
   * representation. Write and read errors can be checked directly
   * on the ostream object after the function call, so no return value is
   * necessary.*/
  void binwrite(std::ostream& out, char               data); 
  void binwrite(std::ostream& out, unsigned char      data); 
  void binwrite(std::ostream& out, unsigned short     data);
  void binwrite(std::ostream& out, unsigned int       data);
  void binwrite(std::ostream& out, unsigned long      data);
  void binwrite(std::ostream& out, size_t             data);
  void binwrite(std::ostream& out, unsigned long long data);
  void binwrite(std::ostream& out, std::string const& data);
  void binwrite(std::ostream& out, float              data); 

  void binread(std::istream& in, char               &data); 
  void binread(std::istream& in, unsigned char      &data); 
  void binread(std::istream& in, unsigned short     &data);
  void binread(std::istream& in, unsigned int       &data);
  void binread(std::istream& in, unsigned long      &data);
  void binread(std::istream& in, size_t             &data);
  void binread(std::istream& in, unsigned long long &data);
  void binread(std::istream& in, std::string        &data);
  void binread(std::istream& in, float              &data); 

  std::ostream& write(std::ostream& out, char x);
  std::ostream& write(std::ostream& out, unsigned char x);
  std::ostream& write(std::ostream& out, short x);
  std::ostream& write(std::ostream& out, unsigned short x);
  std::ostream& write(std::ostream& out, long x);
  std::ostream& write(std::ostream& out, size_t x);
  std::ostream& write(std::ostream& out, float x);

  std::istream& read(std::istream& in, char& x);
  std::istream& read(std::istream& in, unsigned char& x);
  std::istream& read(std::istream& in, short& x);
  std::istream& read(std::istream& in, unsigned short& x);
  std::istream& read(std::istream& in, long& x);
  std::istream& read(std::istream& in, size_t& x);
  std::istream& read(std::istream& in, float& x);

  template<typename WHATEVER>
  char const* 
  binread(char const* p, WHATEVER* buf);

  template<typename numtype>
  char const* 
  binread(char const* p, numtype& buf);

  template<typename K, typename V>
  void binwrite(std::ostream& out, std::pair<K,V> const& data);

  template<typename K, typename V>
  void binread(std::istream& in, std::pair<K,V>& data);

  template<typename K, typename V>
  char const* binread(char const* p, std::pair<K,V>& data);

  template<typename V>
  char const* binread(char const* p, std::vector<V>& v);


  template<typename K, typename V>
  char const* binread(char const* p, std::pair<K,V>& data)
  {
#ifdef VERIFY_TIGHT_PACKING
    assert(p);
#endif
    p = binread(p,data.first);
    p = binread(p,data.second);
    return p;
  }

  template<typename V>
  char const* binread(char const* p, std::vector<V>& v)
  {
    size_t vsize;
#ifdef VERIFY_TIGHT_PACKING
    assert(p);
#endif
    p = binread(p,vsize);
    v.resize(vsize);
    for (size_t i = 0; i < vsize; ++i)
      p = binread(p,v[i]);
    return p;
  }
  
  template<typename T>
  T read(std::istream& in)
  {
    T ret;
    read(in,ret);
    return ret;
  }

  template<typename T>
  T binread(std::istream& in)
  {
    T ret;
    binread(in,ret);
    return ret;
  }


  template<typename T>
  void 
  binwrite(std::ostream& out, std::vector<T> const& data)
  {
    binwrite(out,data.size());
    for (size_t i = 0; i < data.size(); i++)
      { binwrite(out,data[i]); }
  }

  template<typename T>
  void 
  binread(std::istream& in, std::vector<T>& data)
  {
    size_t s;
    binread(in,s);
    data.resize(s);
    for (size_t i = 0; i < s; i++)
      { binread(in,data[i]); }
  }

  template<typename K, typename V>
  void
  binread(std::istream& in, std::map<K,V>& data)
  {
    size_t s; K k; V v;
    binread(in,s);
    data.clear(); 
    // I have no idea why this is necessary, but it is, even when 
    // /data/ is supposed to be empty
    for (size_t i = 0; i < s; i++)
      {
	binread(in,k);
	binread(in,v);
	data[k] = v;
	// cerr << "* " << i << " " << k << " " << v << endl;
      }
  }

  template<typename K, typename V>
  void
  binwrite(std::ostream& out, std::map<K,V> const& data)
  {
    binwrite(out,data.size());
    for (typename std::map<K,V>::const_iterator m = data.begin(); 
	 m != data.end(); m++)
      {
	binwrite(out,m->first);
	binwrite(out,m->second);
      }
  }

  template<typename K, typename V>
  void
  binwrite(std::ostream& out, std::pair<K,V> const& data)
  {
    binwrite(out,data.first);
    binwrite(out,data.second);
  }

  template<typename K, typename V>
  void
  binread(std::istream& in, std::pair<K,V>& data)
  {
    binread(in,data.first);
    binread(in,data.second);
  }


  template<typename WHATEVER>
  char const* 
  binread(char const* p, WHATEVER* buf)
  {
#ifdef VERIFY_TIGHT_PACKING
    assert(p);
#endif
    return binread(p,*buf);
  }

  template<typename numtype>
  char const* 
  binread(char const* p, numtype& buf);
  
} // end namespace ugdiss
#endif
