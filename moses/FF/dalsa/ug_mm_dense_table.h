// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once

#include <boost/iostreams/device/mapped_file.hpp>
#include <stdint.h>
#include <string>

namespace ug { 
namespace mmTable {

template<class celltype, unsigned int dims>
class 
SubTable
{
protected:  
public:
  celltype* m_data;   // where the overall data block starts
  uint64_t* m_blocks; // block size of dims-1-dimensional subtable
  SubTable() : m_data(NULL), m_blocks(NULL) { }
public:
  SubTable(celltype* data, uint64_t* blocks) 
    : m_data(data), m_blocks(blocks) 
  {}

  SubTable<celltype, dims-1> 
  operator[](uint64_t idx);

  SubTable<celltype, dims-1> const 
  operator[](uint64_t idx) const;

  uint64_t size(int i=0) const 
  { 
    if (i >= dims) throw "Out of range.";
    if (i + 1 < dims)
      return m_blocks[i] / m_blocks[i+1]; 
    else return m_blocks[i];
  }

};

template<class celltype>
class 
SubTable<celltype, 1>
{
protected:  
public:
  celltype* m_data; // 
  uint64_t* m_blocks; // size in sub-dimensions
  SubTable() : m_data(NULL), m_blocks(NULL) {}
public:
  SubTable(SubTable const& other) 
    : m_data(other.m_data), m_blocks(other.m_blocks)
  { }

  SubTable(celltype* data, uint64_t* blocksize) 
    : m_data(data), m_blocks(blocksize)
  { }

  uint64_t size(int i=0) const { return m_blocks[0]; }


  celltype& 
  operator[](uint64_t const idx)
  {
    if (m_data == NULL) 
      throw "Table not bound to a file";
    return m_data[idx];
  }

  celltype const& 
  operator[](uint64_t const idx) const
  {
    if (m_data == NULL) 
      throw "Table not bound to a file";
    return m_data[idx];
  }
};

template<class celltype, unsigned int dims>
SubTable<celltype,dims - 1>
SubTable<celltype,dims>::
operator[](uint64_t const idx)
{
  if (m_blocks == NULL) 
    throw "Table not bound to a file";
  SubTable<celltype,dims - 1> S(m_data + idx * m_blocks[1], m_blocks + 1);
  return S;
}

template<class celltype, unsigned int dims>
SubTable<celltype,dims - 1> const
SubTable<celltype,dims>::
operator[](uint64_t const idx) const
{
  if (m_blocks == NULL) 
    throw "Table not bound to a file";
  SubTable<celltype,dims - 1> S(m_data + idx * m_blocks[1], m_blocks + 1);
  return S;
}

template<class celltype, unsigned int dims>
class mmTable : public SubTable<celltype, dims>
{
public:
  // typedef celltype celltype;
  boost::iostreams::mapped_file m_file;
  typedef boost::iostreams::mapped_file_base::mapmode file_flags;
  std::string m_path;
public:
  static file_flags priv, readwrite, readonly;

  mmTable() : SubTable<celltype,dims>::SubTable() {}
  
  std::string const& 
  path() const
  {
    return m_path;
  }

  void 
  open(std::string const& path, 
       file_flags flags = boost::iostreams::mapped_file::priv)
  {
    boost::iostreams::mapped_file_params param;
    param.path = m_path = path;
    param.flags = flags;
    try 
      {
        m_file.open(param);
      } 
    catch (...)
      {
        fprintf(stderr, "Could not open file %s\n", path.c_str());
      }
    assert(m_file.data());
#if 0
    std::cerr << "Opened " << path << " with " << m_file.size() << " " 
              << " bytes at " << __FILE__ << ":"<< __LINE__ << std::endl;
#endif
    this->m_blocks = reinterpret_cast<uint64_t*>(m_file.data());
    this->m_data   = reinterpret_cast<celltype*>(this->m_blocks + dims);
  }

  void 
  create(std::string const& path, uint64_t const* dimensions)
  {
    boost::iostreams::mapped_file_params param;
    param.path = m_path = path;
    param.flags = boost::iostreams::mapped_file::readwrite;
    param.new_file_size = sizeof(celltype);
    for (size_t i = 0; i < dims; ++i) 
      param.new_file_size *= dimensions[i];
    param.new_file_size += (dims) * sizeof(uint64_t);
    m_file.open(param);
  
#if 0
    fprintf(stderr,"Created %s with %'zu bytes (%zu",
            m_path.c_str(), m_file.size(),dimensions[0]);
    for (size_t i = 1; i < dims; ++i)
      fprintf(stderr,"x%zu",dimensions[i]);
    fprintf(stderr," cells with %zu bytes each) at %s:%d\n",
            sizeof(celltype), __FILE__, __LINE__);
#endif

    this->m_blocks = reinterpret_cast<uint64_t*>(m_file.data());
    
    assert(this->m_blocks != NULL);

    this->m_data   = reinterpret_cast<celltype*>(this->m_blocks + dims);
    this->m_blocks[dims-1] = dimensions[dims-1];
    for (int i = dims - 2; i >= 0; --i)
      this->m_blocks[i] = dimensions[i] * this->m_blocks[i+1];

    // I have no idea why closing and opening is necessary
    // but it doesn't seem to work without it.
    m_file.close(); 
    open(path, boost::iostreams::mapped_file::readwrite);

    // try this with and without closing and re-opening to demonstrate the problem

    // #include <algorithm>
    // #include <iostream>
    // #include "ug_mm_dense_table.h"
    // using namespace std;
    // int main()
    // {
    //   ug::mmTable::mmTable<int,2> T;
    //   uint64_t dimensions[] = { 10, 10 };
    //   T.create("foo.dat", dimensions);
    //   T[3][3] = 3;
    //   cout << T[3][3] << endl;
    //   T.close();
    //   T.open("foo.dat");
    //   cout << T[3][3] << endl;
    //   T[3][3] = 3;
    //   cout << T[3][3] << endl;
    //   T.close();
    //   T.open("foo.dat");
    //   cout << T[3][3] << endl;
    //   T.close();
    // }
  }

  void 
  close()
  {
    m_file.close();
    this->m_data = NULL;
    this->m_blocks = NULL;
    this->m_path = "";
  }
  
};

template<class celltype, unsigned int dims>
boost::iostreams::mapped_file_base::mapmode
mmTable<celltype, dims>::
priv;

template<class celltype, unsigned int dims>
boost::iostreams::mapped_file_base::mapmode
mmTable<celltype, dims>::
readonly;

template<class celltype, unsigned int dims>
boost::iostreams::mapped_file_base::mapmode
mmTable<celltype, dims>::
readwrite;
}} // end namespaces
