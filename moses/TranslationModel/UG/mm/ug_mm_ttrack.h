// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// Memory-mapped corpus track. The corpus (each Token occupying a fixed number
// of bytes (must be compatible with the memory alignment in the OS) is stored
// as one huge array. The "index" maps from sentence IDs to positions within
// that array.

// (c) 2007-2010 Ulrich Germann. All rights reserved

#ifndef __ug_mm_ttrack
#define __ug_mm_ttrack

#include <sstream>
#include <string>
#include <stdexcept>

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/shared_ptr.hpp>

#include "tpt_typedefs.h"
#include "ug_typedefs.h"
#include "tpt_tokenindex.h"
#include "ug_ttrack_base.h"
#include "num_read_write.h"
#include "ug_load_primer.h"
#include "ug_tsa_base.h"

namespace sapt
{
  namespace bio=boost::iostreams;

  template<typename TKN=id_type>
  class mmTtrack : public Ttrack<TKN>
  {
  public:
    typedef TKN Token;

  private:
    id_type numSent;
    id_type numWords;
    bio::mapped_file_source file;
    Token   const* data;  // pointer to first word of first sentence
    id_type const* index; /* pointer to index (change data type for corpora
			   * of more than four billion words)
			   */
  public:
    mmTtrack(std::string fname);
    mmTtrack();

    // return pointer to beginning of sentence
    Token const* sntStart(size_t sid) const;

    // return pointer to end of sentence
    Token const* sntEnd(size_t sid) const;

    // return size of corpus (in number of sentences)
    size_t size() const;

    // return size of corpus (in number of tokens)
    size_t numTokens() const;

    // open an mmTtrack file
    void open(std::string fname);

    //  FUNCTIONS FOR BUILDING CORPUS TRACKS
    // write a blank file header at the beginning of a new ttrack file
    void write_blank_file_header(std::ostream& out) const;

    // write the sentence index /idx/ and fill the file header
    void write_index_and_finalize(std::ostream& out,
				  std::vector<id_type> const& idx,
				  count_type tokenCount) const;

    // copy a contiguous sequence of sentences to another stream
    // return the number of tokens copied
    id_type copySentences(std::ostream& trg, id_type start, id_type stop) const;

    /** find the sentence id of a given token */
    id_type findSid(TKN const* t) const;

    id_type findSid(id_type tokenOffset) const;

    /// re-assign ids based on the id maps in /f/
    void remap(std::string const fname, std::vector<id_type const*> const & f) const;

  };

  /// re-assign ids based on the id maps in /f/
  template<typename TKN>
  void
  mmTtrack<TKN>::
  remap(std::string const fname, std::vector<id_type const*> const & f) const
  {
    bio::mapped_file myfile(fname);
    assert(myfile.is_open());
    Moses::prime(myfile);
    tpt::filepos_type idxOffset;
    const char* p = myfile.data();
    id_type numSent,numWords;
    p = tpt::numread(p,idxOffset);
    p = tpt::numread(p,numSent);
    p = tpt::numread(p,numWords);
    data  = reinterpret_cast<TKN*>(p);
    for (size_t i = 0; i < numWords; ++i)
      data[i] = data[i].remap(f);
    myfile.close();
  }


  template<typename TKN>
  size_t
  mmTtrack<TKN>::
  size() const
  {
    return this->numSent;
  }

  template<typename TKN>
  size_t
  mmTtrack<TKN>::
  numTokens() const
  {
    return this->numWords;
  }

  template<typename TKN>
  TKN const*
  mmTtrack<TKN>::
  sntStart(size_t sid) const // return pointer to beginning of sentence
  {
    if (sid >= this->numSent)
      {
	std::cerr << "Fatal error: requested sentence #"
		  << sid <<" is beyond corpus size ("
		  << this->numSent <<")" << std::endl;
      }
    assert(sid < this->numSent);
    return data+index[sid];
  }

  template<typename TKN>
  TKN const*
  mmTtrack<TKN>::
  sntEnd(size_t sid) const // return pointer to end of sentence
  {
    assert(sid < this->numSent);
    return data+index[sid+1];
  }

  template<typename TKN>
  mmTtrack<TKN>::
  mmTtrack()
  {
    data  = NULL;
    index = NULL;
    this->numSent = this->numWords = 0;
  }

  template<typename TKN>
  mmTtrack<TKN>::
  mmTtrack(std::string fname)
  {
    open(fname);
  }

  template<typename TKN>
  void
  mmTtrack<TKN>::
  open(std::string fname)
  {
    if (access(fname.c_str(),F_OK))
      {
	std::ostringstream msg;
        msg << "mmTtrack<>::open: File '" << fname << "' does not exist.";
        throw std::runtime_error(msg.str().c_str());
      }
    file.open(fname);
    if (!file.is_open())
      {
	std::cerr << "Error opening file " << fname << std::endl;
	assert(0);
      }
    tpt::filepos_type idxOffset;
    char const* p = file.data();
    p = tpt::numread(p, idxOffset);
    p = tpt::numread(p,this->numSent);
    p = tpt::numread(p,this->numWords);
    data  = reinterpret_cast<Token const*>(p);
    index = reinterpret_cast<id_type const*>(file.data()+idxOffset);
  }

  template<typename TKN>
  id_type
  mmTtrack<TKN>::
  findSid(TKN const* t) const
  {
    id_type tokenPos = t-data;
    id_type const* p = std::upper_bound(index,index+this->numSent,tokenPos);
    assert(p>index);
    return p-index-1;
  }

  template<typename TKN>
  id_type
  mmTtrack<TKN>::
  findSid(id_type tokenPos) const
  {
    id_type const* p = std::upper_bound(index,index+this->numSent,tokenPos);
    assert(p>index);
    return p-index-1;
  }

  template<typename TKN>
  void
  mmTtrack<TKN>::
  write_blank_file_header(std::ostream& out) const
  {
    tpt::numwrite(out,filepos_type(0)); // place holder for index start
    tpt::numwrite(out,id_type(0));      // place holder for index size
    tpt::numwrite(out,id_type(0));      // place holder for token count
  }

  template<typename TKN>
  void
  mmTtrack<TKN>::
  write_index_and_finalize(std::ostream& out,
			   std::vector<id_type>const& idx,
			   id_type tokenCount) const
  {
    id_type       idxSize = idx.size();
    tpt::filepos_type idxStart = out.tellp();
    for (size_t i = 0; i < idx.size(); ++i)
      tpt::numwrite(out,idx[i]);
    out.seekp(0);
    tpt::numwrite(out,idxStart);
    tpt::numwrite(out,idxSize-1);
    tpt::numwrite(out,tokenCount);
  }

  template<typename TKN>
  id_type
  mmTtrack<TKN>::
  copySentences(std::ostream& trg, id_type start, id_type stop) const
  {
    assert(stop > start);
    TKN const* a = sntStart(start);
    TKN const* z = sntEnd(stop-1);
    size_t len = (z-a)*sizeof(TKN);
    if (!len) return 0;
    trg.write(reinterpret_cast<char const*>(a),len);
    return z-a;
  }

}
#endif
