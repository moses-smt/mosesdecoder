// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// TO DO (12.01.2011):
//
// - Vocab items should be stored in order of ids, so that we can
//   determine their length by taking computing V[id+1] - V[id]
//   instead of using strlen.
//
// (c) 2007,2008 Ulrich Germann

#ifndef __ugTokenIndex_hh
#define __ugTokenIndex_hh
#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include "tpt_typedefs.h"
#include <vector>
#include <map>

namespace bio=boost::iostreams;

namespace sapt
{
  class TokenIndex
  {
    typedef tpt::id_type id_type;
    /** Reverse index: maps from ID to char const* */
    mutable std::vector<char const*> ridx;
    /** Label for the UNK token */
    std::string unkLabel;
    id_type unkId,numTokens;

    /// New 2013-09-02: thread-safe
    boost::scoped_ptr<boost::mutex> lock;

    // NEW 2011-01-30: dynamic adding of unknown items
    bool dynamic; // dynamically assign a new word id to unknown items?
    boost::shared_ptr<std::map<std::string, tpt::id_type> >   str2idExtra;
    boost::shared_ptr<std::vector<std::string> > newWords;
    // The use of pointers to external items is a bit of a bad hack
    // in terms of the semantic of TokenIndex const: since external items
    // are changed, the TokenIndex instance remains unchanged and const works,
    // even though in reality the underlying object on the coceptual level
    // *IS* changed. This means that dynamic TokenIndex instances are not
    // thread-safe!

  public:
    /** string->ID lookup works via binary search in a std::vector of Entry instances */
    class Entry
    {
    public:
      uint32_t offset;
      id_type id;
    };

    /** Comparison function object used for Entry instances */
    class CompFunc
    {
    public:
      char const* base;
      CompFunc();
      bool operator()(Entry const& A, char const* w);
    };

    bio::mapped_file_source file;
    Entry const* startIdx;
    Entry const* endIdx;
    CompFunc comp;
    TokenIndex(std::string unkToken="UNK");
    // TokenIndex(std::string fname,std::string unkToken="UNK",bool dyna=false);
    void open(std::string fname,std::string unkToken="UNK",bool dyna=false);
    void close();
    // id_type unkId,numTokens;
    id_type operator[](char const* w)  const;
    id_type operator[](std::string const& w)  const;
    char const* const operator[](id_type id) const;
    char const* const operator[](id_type id);
    std::vector<char const*> reverseIndex() const;

    std::string toString(std::vector<id_type> const& v);
    std::string toString(std::vector<id_type> const& v) const;

    std::string toString(id_type const* start, id_type const* const stop);
    std::string toString(id_type const* start, id_type const* const stop) const;

    std::vector<id_type> toIdSeq(std::string const& line) const;

    bool fillIdSeq(std::string const& line, std::vector<id_type> & v) const;

    void iniReverseIndex();
    id_type getNumTokens() const;
    id_type getUnkId() const;

    // the following two functions are deprecated; use ksize() and tsize() instead
    id_type knownVocabSize() const; // return size of known (fixed) vocabulary
    id_type totalVocabSize() const; // total of known and dynamically items

    id_type ksize() const; // shorthand for knownVocabSize();
    id_type tsize() const; // shorthand for totalVocabSize();


    char const* const getUnkToken() const;

    void write(std::string fname); // write TokenIndex to a new file
    bool isDynamic() const;
    bool setDynamic(bool onoff);

    void setUnkLabel(std::string unk);
  };

  void
  write_tokenindex_to_disk(std::vector<std::pair<std::string,uint32_t> > const& tok,
                           std::string const& ofile, std::string const& unkToken);

  /** for sorting words by frequency */
  class compWords
  {
    std::string unk;
  public:
    compWords(std::string _unk) : unk(_unk) {};

    bool
    operator()(std::pair<std::string,size_t> const& A,
               std::pair<std::string,size_t> const& B) const
    {
      if (A.first == unk) return false;// do we still need this special treatment?
      if (B.first == unk) return true; // do we still need this special treatment?
      if (A.second == B.second)
        return A.first < B.first;
      return A.second > B.second;
    }
  };

  template<class MYMAP>
  void
  mkTokenIndex(std::string ofile,MYMAP const& M,std::string unkToken)
  {
    // typedef std::pair<uint32_t,id_type> IndexEntry; // offset and id
    typedef std::pair<std::string,uint32_t>  Token;      // token and id


    // first, sort the word list in decreasing order of frequency, so that we
    // can assign IDs in an encoding-efficient manner (high frequency. low ID)
    std::vector<std::pair<std::string,size_t> > wcounts(M.size()); // for sorting by frequency
    typedef typename MYMAP::const_iterator myIter;
    size_t z=0;
    for (myIter m = M.begin(); m != M.end(); m++)
      {
	// cout << m->first << " " << m->second << std::endl;
	wcounts[z++] = std::pair<std::string,size_t>(m->first,m->second);
      }
    compWords compFunc(unkToken);
    sort(wcounts.begin(),wcounts.end(),compFunc);

    // Assign IDs ...
    std::vector<Token> tok(wcounts.size());
    for (size_t i = 0; i < wcounts.size(); i++)
      tok[i] = Token(wcounts[i].first,i);
    // and re-sort in alphabetical order
    sort(tok.begin(),tok.end());
    write_tokenindex_to_disk(tok,ofile,unkToken);
  }

  template<typename Token>
  void
  fill_token_seq(TokenIndex& V, std::string const& line, std::vector<Token>& dest)
  {
    std::istringstream buf(line); std::string w;
    while (buf>>w) dest.push_back(Token(V[w]));
  }
}
#endif
