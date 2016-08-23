// $Id$

/* ---------------------------------------------------------------- */
/* Copyright 2005 (c) by RWTH Aachen - Lehrstuhl fuer Informatik VI */
/* Richard Zens                                                     */
/* ---------------------------------------------------------------- */
#ifndef moses_PrefixTree_h
#define moses_PrefixTree_h

#include <vector>
#include <algorithm>
#include <deque>
#include "Util.h"
#include "FilePtr.h"
#include "File.h"

namespace Moses
{

/** @todo How is this used in the pb binary phrase table?
 */
template<typename T,typename D>
class PrefixTreeSA
{
public:
  typedef T Key;
  typedef D Data;

  typedef PrefixTreeSA<T,D> Self;
  typedef std::vector<T> VT;
  typedef std::vector<Self*> VP;
  typedef std::vector<D> VD;

  VT keys;
  VP ptr;
  VD data;

  static Data def;

public:
  PrefixTreeSA() {}

  ~PrefixTreeSA() {
    for(size_t i=0; i<ptr.size(); ++i) delete ptr[i];
  }

  static const Data& getDefault() {
    return def;
  }
  static void setDefault(const Data& x) {
    def=x;
  }


  // insert sequence
  template<typename fwiter> Data& insert(fwiter b,fwiter e) {
    typename VT::iterator i=std::lower_bound(keys.begin(),keys.end(),*b);
    typename VT::iterator kb=keys.begin();
    size_t pos=std::distance(kb,i);

    if(i==keys.end() || *i!=*b) {
      keys.insert(i,*b);
      data.insert(data.begin()+pos,def);

      Self *self = NULL;
      ptr.insert(ptr.begin()+pos, self);
    }
    if(++b!=e) {
      if(!ptr[pos]) ptr[pos]=new Self;
      return ptr[pos]->insert(b,e);
    } else return data[pos];
  }
  // insert container
  template<typename cont> Data& insert(const cont& c) {
    return insert(c.begin(),c.end());
  }

  size_t size() const {
    return keys.size();
  }
  const Key& getKey(size_t i) const {
    return keys[i];
  }
  const Data& getData(size_t i) const {
    return data[i];
  }
  const Self* getPtr(size_t i) const {
    return ptr[i];
  }

  size_t findKey(const Key& k) const {
    typename VT::const_iterator i=std::lower_bound(keys.begin(),keys.end(),k);
    if(i==keys.end() || *i!=k) return keys.size();
    return std::distance(keys.begin(),i);
  }

  // find sequence
  template<typename fwiter> const Data* findPtr(fwiter b,fwiter e) const {
    size_t pos=findKey(*b);
    if(pos==keys.size()) return 0;
    if(++b==e) return &data[pos];
    if(ptr[pos]) return ptr[pos]->findPtr(b,e);
    else return 0;
  }
  // find container
  template<typename cont> const Data* findPtr(const cont& c) const {
    return findPtr(c.begin(),c.end());
  }


  // find sequence
  template<typename fwiter> const Data& find(fwiter b,fwiter e) const {
    if(const Data* p=findPtr(b,e)) return *p;
    else return def;
  }

  // find container
  template<typename cont> const Data& find(const cont& c) const {
    return find(c.begin(),c.end());
  }

  void shrink() {
    ShrinkToFit(keys);
    ShrinkToFit(ptr);
    ShrinkToFit(data);
  }

};
template<typename T,typename D> D PrefixTreeSA<T,D>::def;

/////////////////////////////////////////////////////////////////////////////

/** @todo How is this used in the pb binary phrase table?
 */
template<typename T,typename D>
class PrefixTreeF
{
public:
  typedef T Key;
  typedef D Data;
private:
  typedef PrefixTreeF<Key,Data> Self;
public:
  typedef FilePtr<Self> Ptr;
private:
  typedef std::vector<Key> VK;
  typedef std::vector<Data> VD;
  typedef std::vector<Ptr> VP;

  VK keys;
  VD data;
  VP ptr;

  static Data def;

  OFF_T startPos;
  FILE* f;
public:

  PrefixTreeF(FILE* f_=0) : f(f_) {
    if(f) read();
  }

  ~PrefixTreeF() {
    free();
  }

  void read() {
    startPos=fTell(f);
    fReadVector(f,keys);
    fReadVector(f,data);
    ptr.clear();
    ptr.resize(keys.size());
    std::vector<OFF_T> rawOffs(keys.size());
    size_t bytes_read = fread(&rawOffs[0], sizeof(OFF_T), keys.size(), f);
    UTIL_THROW_IF2(bytes_read != keys.size(), "Read error at " << HERE);
    for(size_t i=0; i<ptr.size(); ++i)
      if (rawOffs[i]) ptr[i].set(f, rawOffs[i]);
  }

  void free() {
    for(typename VP::iterator i=ptr.begin(); i!=ptr.end(); ++i) i->free();
  }

  void reserve(size_t s) {
    keys.reserve(s);
    data.reserve(s);
    ptr.reserve(s);
  }

  template<typename fwiter>
  void changeData(fwiter b,fwiter e,const Data& d) {
    typename VK::const_iterator i=std::lower_bound(keys.begin(),keys.end(),*b);
    if(i==keys.end() || *i!=*b) {
      TRACE_ERR("ERROR: key not found in changeData!\n");
      return;
    }
    typename VK::const_iterator kb=keys.begin();
    size_t pos=std::distance(kb,i);
    if(++b==e) {
      OFF_T p=startPos+keys.size()*sizeof(Key)+2*sizeof(unsigned)+pos*sizeof(Data);
      TRACE_ERR("elem found at pos "<<p<<" old val: "<<data[pos]<<"  startpos: "<<startPos<<"\n");
      if(data[pos]!=d) {
        data[pos]=d;
        fSeek(f,p);
        fWrite(f,d);
      }
      return;
    }
    if(ptr[pos]) ptr[pos]->changeData(b,e,d);
    else {
      TRACE_ERR("ERROR: seg not found!in changeData\n");
    }
  }


  void create(const PrefixTreeSA<Key,Data>& psa,const std::string& fname) {
    FILE* f=fOpen(fname.c_str(),"wb");
    create(psa,f);
    fclose(f);
  }

  void create(const PrefixTreeSA<Key,Data>& psa,FILE* f,int verbose=0) {
    setDefault(psa.getDefault());

    typedef std::pair<const PrefixTreeSA<Key,Data>*,OFF_T> P;
    typedef std::deque<P> Queue;

    Queue queue;

    queue.push_back(P(&psa,fTell(f)));
    bool isFirst=1;
    size_t ns=1;
    while(queue.size()) {
      if(verbose && queue.size()>ns) {
        TRACE_ERR("stack size in PF create: "<<queue.size()<<"\n");
        while(ns<queue.size()) ns*=2;
      }
      const P& pp=queue.back();
      const PrefixTreeSA<Key,Data>& p=*pp.first;
      OFF_T pos=pp.second;
      queue.pop_back();

      if(!isFirst) {
        OFF_T curr=fTell(f);
        fSeek(f,pos);
        fWrite(f,curr);
        fSeek(f,curr);
      } else isFirst=0;

      size_t s=0;
      s+=fWriteVector(f,p.keys);
      s+=fWriteVector(f,p.data);

      for(size_t i=0; i<p.ptr.size(); ++i) {
        if(p.ptr[i])
          queue.push_back(P(p.ptr[i],fTell(f)));
        OFF_T ppos=0;
        s+=fWrite(f,ppos);
      }
    }
  }

  size_t size() const {
    return keys.size();
  }
  const Key& getKey(size_t i) const {
    return keys[i];
  }
  const Data& getData(size_t i) const {
    return data[i];
  }
  const Self* getPtr(size_t i) const {
    return ptr[i];
  }

  size_t findKey(const Key& k) const {
    typename VK::const_iterator i=std::lower_bound(keys.begin(),keys.end(),k);
    if(i==keys.end() || *i!=k) return keys.size();
    return std::distance(keys.begin(),i);
  }

  Ptr const* findKeyPtr(const Key& k) const {
    size_t pos=findKey(k);
    return (pos<keys.size() ? &ptr[pos] : 0);
  }

  // find sequence
  template<typename fwiter> const Data* findPtr(fwiter b,fwiter e) const {
    typename VK::const_iterator i=std::lower_bound(keys.begin(),keys.end(),*b);
    if(i==keys.end() || *i!=*b) return 0;
    size_t pos=std::distance(keys.begin(),i);
    if(++b==e) return &data[pos];
    if(ptr[pos]) return ptr[pos]->findPtr(b,e);
    else return 0;
  }
  // find container
  template<typename cont> const Data* findPtr(const cont& c) const {
    return findPtr(c.begin(),c.end());
  }


  // find sequence
  template<typename fwiter> const Data& find(fwiter b,fwiter e) const {
    if(const Data* p=findPtr(b,e)) return *p;
    else return def;
  } //return (p?*p:def);}

  // find container
  template<typename cont> const Data& find(const cont& c) const {
    return find(c.begin(),c.end());
  }

  static void setDefault(const Data& d) {
    def=d;
  }
  static const Data& getDefault() {
    return def;
  }


  void print(std::ostream& out,const std::string s="") const {

    out<<s<<"startpos: "<<startPos<<"  size: "<<keys.size()<<"\n";
    for(size_t i=0; i<keys.size(); ++i) {
      out<<s<<i<<" - "<<keys[i]<<" "<<data[i]<<"\n";
    }
    for(size_t i=0; i<ptr.size(); ++i)
      if(ptr[i])
        ptr[i]->print(out,s+"  ");
  }


};
template<typename T,typename D> D PrefixTreeF<T,D>::def;

}

#endif
