// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// (c) 2007-2013 Ulrich Germann
#include <sstream>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <boost/pool/pool_alloc.hpp>

#include "tpt_tokenindex.h"
#include "ug_typedefs.h"

using namespace std;
namespace sapt
{

  TokenIndex::
  TokenIndex(string unkToken)
    : ridx(0), unkLabel(unkToken), unkId(1), numTokens(0)
    , startIdx(0), endIdx(0)
  {
    lock.reset(new boost::mutex());
  };

#if 0
  TokenIndex::
  TokenIndex(string fname, string unkToken,bool dyna)
    : ridx(0),unkLabel(unkToken)
  {
    this->open(fname,unkToken,dyna);
  };
#endif

  void
  TokenIndex::
  open(string fname, string unkToken,bool dyna)
  {
    if (access(fname.c_str(),F_OK))
      {
        ostringstream msg;
        msg << "TokenIndex::open: File '" << fname << "' does not exist.";
        throw std::runtime_error(msg.str().c_str());
      }

    file.open(fname);
    if (!file.is_open())
      {
        ostringstream msg;
        msg << "TokenIndex::open: Error opening file '" << fname << "'.";
        throw std::runtime_error(msg.str().c_str());
      }

    this->numTokens = *(reinterpret_cast<uint32_t const*>(file.data()));
    unkId = *(reinterpret_cast<id_type const*>(file.data()+4));

    startIdx = reinterpret_cast<Entry const*>(file.data()+4+sizeof(id_type));
    endIdx   = startIdx + numTokens;
    comp.base = reinterpret_cast<char const*>(endIdx);
    if (!unkToken.empty())
      {
        Entry const* bla = lower_bound(startIdx,endIdx,unkToken.c_str(),comp);
        unkId = ((bla < endIdx && unkToken == comp.base+bla->offset)
                 ? bla->id
                 : numTokens);
      }
    this->dynamic=dyna;
    if (dyna)
      {
        this->str2idExtra.reset(new map<string,id_type>());
        this->newWords.reset(new vector<string>());
      }
  }

  void
  TokenIndex::
  close()
  {
    file.close();
  }

  TokenIndex::
  CompFunc::
  CompFunc()
  {};

  bool
  TokenIndex::
  CompFunc::
  operator()(Entry const& A, char const* w)
  {
    return strcmp(base+A.offset,w) < 0;
  };

  id_type
  TokenIndex::
  operator[](char const* p) const
  {
    if (startIdx != endIdx)
      {
        Entry const* bla = lower_bound(startIdx,endIdx,p,comp);
        if (bla != endIdx && !strcmp(comp.base+bla->offset,p))
          return bla->id;
        if (!dynamic) return unkId;
      }
    else if (!dynamic) return strcmp(p,"NULL") && unkId;
    
    boost::lock_guard<boost::mutex> lk(*this->lock);
    // stuff below is new as of 2011-01-30, for dynamic adding of
    // unknown items IMPORTANT: numTokens is not currently not
    // changed, it is the number of PRE-EXISING TOKENS, not including
    // dynamically added Items
    // if (!str2idExtra)
    //   {
    //     this->str2idExtra.reset(new map<string,id_type>());
    //     this->newWords.reset(new vector<string>());
    //   }
    map<string,id_type>::value_type newItem(p,str2idExtra->size()+numTokens);
    pair<map<string,id_type>::iterator,bool> foo = str2idExtra->insert(newItem);
    if (foo.second) // it actually is a new item
      newWords->push_back(foo.first->first);
    return foo.first->second;
  }

  id_type
  TokenIndex::
  operator[](string const& w) const
  {
    return (*this)[w.c_str()];
  }

  vector<char const*>
  TokenIndex::
  reverseIndex() const
  {
    size_t numToks = endIdx-startIdx;

    // cout << "tokenindex has " << numToks << " tokens" << endl;

    vector<char const*> v(numToks,NULL);
    // v.reserve(endIdx-startIdx);
    for (Entry const* x = startIdx; x != endIdx; x++)
      {
	if (x->id >= v.size())
	  v.resize(x->id+1);
	v[x->id] = comp.base+x->offset;
      }
    // cout << "done reversing index " << endl;
    return v;
  }

  char const* const
  TokenIndex::
  operator[](id_type id) const
  {
    if (!ridx.size())
      {
        boost::lock_guard<boost::mutex> lk(*this->lock);
        // Someone else (multi-threading!) may have created the 
        // reverse index in the meantime, so let's check again
        if (!ridx.size()) ridx = reverseIndex();
      }
    if (id < ridx.size())
      return ridx[id];
    
    boost::lock_guard<boost::mutex> lk(*this->lock);
    if (dynamic && id < ridx.size()+newWords->size())
      return (*newWords)[id-ridx.size()].c_str();
    return unkLabel.c_str();
  }

  void
  TokenIndex::
  iniReverseIndex()
  {
    if (!ridx.size())
      {
        boost::lock_guard<boost::mutex> lk(*this->lock);
        if (!ridx.size()) ridx = reverseIndex();
      }
  }


  char const* const
  TokenIndex::
  operator[](id_type id)
  {
    if (!ridx.size())
      {
        boost::lock_guard<boost::mutex> lk(*this->lock);
        if (!ridx.size()) ridx = reverseIndex();
      }
    if (id < ridx.size())
      return ridx[id];
    boost::lock_guard<boost::mutex> lk(*this->lock);
    if (dynamic && id < ridx.size()+newWords->size())
      return (*newWords)[id-ridx.size()].c_str();
    return unkLabel.c_str();
  }

  string
  TokenIndex::
  toString(vector<id_type> const& v)
  {
    if (!ridx.size())
      {
        boost::lock_guard<boost::mutex> lk(*this->lock);
        if (!ridx.size()) ridx = reverseIndex();
      }
    ostringstream buf;
    for (size_t i = 0; i < v.size(); i++)
      buf << (i ? " " : "") << (*this)[v[i]];
    return buf.str();
  }

  string
  TokenIndex::
  toString(vector<id_type> const& v) const
  {
    if (!ridx.size())
      {
        boost::lock_guard<boost::mutex> lk(*this->lock);
        if (!ridx.size()) ridx = reverseIndex();
      }
    ostringstream buf;
    for (size_t i = 0; i < v.size(); i++)
      buf << (i ? " " : "") << (*this)[v[i]];
    return buf.str();
  }

  string
  TokenIndex::
  toString(id_type const* start, id_type const* const stop)
  {
    if (!ridx.size())
      {
        boost::lock_guard<boost::mutex> lk(*this->lock);
        if (!ridx.size()) ridx = reverseIndex();
      }
    ostringstream buf;
    if (start < stop)
      buf << (*this)[*start];
    while (++start < stop)
      buf << " " << (*this)[*start];
    return buf.str();
  }

  string
  TokenIndex::
  toString(id_type const* start, id_type const* const stop) const
  {
    if (!ridx.size())
      {
        boost::lock_guard<boost::mutex> lk(*this->lock);
        if (!ridx.size()) ridx = reverseIndex();
      }
    ostringstream buf;
    if (start < stop)
      buf << (*this)[*start];
    while (++start < stop)
      buf << " " << (*this)[*start];
    return buf.str();
  }

  vector<id_type>
  TokenIndex::
  toIdSeq(string const& line) const
  {
    istringstream buf(line);
    string w;
    vector<id_type> retval;
    while (buf>>w)
      retval.push_back((*this)[w]);
    return retval;
  }

  /// Return false if line contains unknown tokens, true otherwise
  bool
  TokenIndex::
  fillIdSeq(string const& line, vector<id_type> & v) const
  {
    bool allgood = true; string w;
    v.clear();
    for (istringstream buf(line); buf>>w;)
      {
        v.push_back((*this)[w]);
        allgood = allgood && v.back() > 1;
      }
    return allgood;
  }

  id_type
  TokenIndex::
  getNumTokens() const
  {
    return numTokens;
  }

  id_type
  TokenIndex::
  getUnkId() const
  {
    return unkId;
  }

  char const* const
  TokenIndex::
  getUnkToken() const
  {
    return unkLabel.c_str();
    // return (*this)[unkId];
  }

  id_type
  TokenIndex::
  knownVocabSize() const
  {
    return numTokens;
  }

  id_type
  TokenIndex::
  ksize() const
  {
    return numTokens;
  }

  id_type
  TokenIndex::
  totalVocabSize() const
  { return tsize(); }

  id_type
  TokenIndex::
  tsize() const
  {
    return (newWords != NULL
            ? numTokens+newWords->size()
            : numTokens);
  }

  void
  write_tokenindex_to_disk(vector<pair<string,uint32_t> > const& tok,
                           string const& ofile, string const& unkToken)
  {
    typedef pair<uint32_t,id_type> IndexEntry; // offset and id

    // Write token strings to a buffer, keep track of offsets
    vector<IndexEntry> index(tok.size());
    ostringstream data;
    id_type unkId = tok.size();
    for (size_t i = 0; i < tok.size(); i++)
      {
        if (tok[i].first == unkToken)
          unkId = tok[i].second;
        index[i].first  = data.tellp();   // offset of string
        index[i].second = tok[i].second;  // respective ID
        data<<tok[i].first<<char(0);      // write string to buffer
      }

    // Now write the actual file
    ofstream out(ofile.c_str());
    uint32_t vsize = index.size(); // how many vocab items?
    out.write(reinterpret_cast<char*>(&vsize),4);
    out.write(reinterpret_cast<char*>(&unkId),sizeof(id_type));
    for (size_t i = 0; i < index.size(); i++)
      {
        out.write(reinterpret_cast<char*>(&index[i].first),4);
        out.write(reinterpret_cast<char*>(&index[i].second),sizeof(id_type));
      }
    out<<data.str();
  }

  void
  TokenIndex::
  write(string fname)
  {
    typedef pair<string,uint32_t>  Token;      // token and id
    vector<Token>       tok(totalVocabSize());
    for (id_type i = 0; i < tok.size(); ++i)
      tok[i] = Token((*this)[i],i);
    sort(tok.begin(),tok.end());
    write_tokenindex_to_disk(tok,fname,unkLabel);
  }

  bool
  TokenIndex::
  isDynamic() const
  {
    return dynamic;
  }

  bool
  TokenIndex::
  setDynamic(bool on)
  {
    bool ret = dynamic;
    if (on && this->str2idExtra == NULL)
      {
        this->str2idExtra.reset(new map<string,id_type>());
        this->newWords.reset(new vector<string>());
      }
    dynamic = on;
    if (on)
      {
	(*this)["NULL"];
	(*this)[unkLabel];
      }
    return ret;
  }

  void
  TokenIndex::
  setUnkLabel(string unk)
  {
    unkId = (*this)[unk];
    unkLabel = unk;
  }

}
