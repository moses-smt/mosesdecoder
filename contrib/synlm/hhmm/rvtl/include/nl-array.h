///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// This file is part of ModelBlocks. Copyright 2009, ModelBlocks developers. //
//                                                                           //
//    ModelBlocks is free software: you can redistribute it and/or modify    //
//    it under the terms of the GNU General Public License as published by   //
//    the Free Software Foundation, either version 3 of the License, or      //
//    (at your option) any later version.                                    //
//                                                                           //
//    ModelBlocks is distributed in the hope that it will be useful,         //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of         //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
//    GNU General Public License for more details.                           //
//                                                                           //
//    You should have received a copy of the GNU General Public License      //
//    along with ModelBlocks.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                           //
//    ModelBlocks developers designate this particular file as subject to    //
//    the "Moses" exception as provided by ModelBlocks developers in         //
//    the LICENSE file that accompanies this code.                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#ifndef _NL_ARRAY__
#define _NL_ARRAY__

#include <cassert>
#include <iostream>
using namespace std;

////////////////////////////////////////////////////////////////////////////////

template<class T>
class Array {
 private:

  T* pt;
  unsigned int iNextOpen;
  unsigned int iCapacity;

  static const T tDummy;

 public:

  class const_iterator;
  class iterator : public pair<Array<T>*,int> {
    typedef pair<Array<T>*,int> parent;
   public:
    iterator             ( Array<T>* pat, int i ) : parent(pat,i) { }
    bool      operator== ( const iterator&       it )             { return (parent::first==it.first && parent::second==it.second); }
    bool      operator== ( const const_iterator& it )             { return (parent::first==it.first && parent::second==it.second); }
    bool      operator!= ( const iterator&       it )             { return (parent::first!=it.first || parent::second!=it.second); }
    bool      operator!= ( const const_iterator& it )             { return (parent::first!=it.first || parent::second!=it.second); }
    T*        operator-> ( )                                      { return &parent::first->set(parent::second); }
    iterator& operator++ ( )                                      { ++parent::second; return *this; }
    iterator  next       ( )                                      { return iterator ( parent::first, parent::second+1 ); }
  };
  class const_iterator : public pair<const Array<T>*,int> {
    typedef pair<const Array<T>*,int> parent;
   public:
    const_iterator             ( const iterator& it )         : parent(it)    { }
    const_iterator             ( const Array<T>* pat, int i ) : parent(pat,i) { }
    bool            operator== ( const iterator&       it )                   { return (parent::first==it.first && parent::second==it.second); }
    bool            operator== ( const const_iterator& it )                   { return (parent::first==it.first && parent::second==it.second); }
    bool            operator!= ( const iterator&       it )                   { return (parent::first!=it.first || parent::second!=it.second); }
    bool            operator!= ( const const_iterator& it )                   { return (parent::first!=it.first || parent::second!=it.second); }
    const T*        operator-> ( )                                            { return &parent::first->get(parent::second); }
    const_iterator& operator++ ( )                                            { ++parent::second; return *this; }
    const_iterator  next       ( )                                            { return const_iterator ( parent::first, parent::second+1 ); }
  };

  // Constructor / destructor methods...
  Array ( )                            : pt(new T[1]), iNextOpen(0), iCapacity(1) { }
  Array ( unsigned int i )             : pt(new T[i]), iNextOpen(0), iCapacity(i) { }
  Array ( unsigned int i, const T& t ) : pt(new T[i]), iNextOpen(0), iCapacity(i) { for(unsigned int j=0; j<iCapacity; j++) pt[j]=t; }
  Array ( const Array<T>& at )         : pt(new T[at.iCapacity]), iNextOpen(at.iNextOpen), iCapacity(at.iCapacity)
    { for(unsigned int i=0; i<iNextOpen; i++) pt[i]=at.pt[i]; }
  ~Array() { delete[] pt; }

  // Specification methods...
  Array<T>& operator= ( const Array<T>& at ) { delete[] pt; pt=new T[at.iCapacity]; iNextOpen=at.iNextOpen; iCapacity=at.iCapacity;
                                               for(unsigned int i=0; i<iNextOpen; i++) pt[i]=at.pt[i]; return *this; }
  void clear          ( )                { iNextOpen=0; }
  void clearEnd       ( unsigned int i ) { iNextOpen=i; }
  T&   add            ( )                { return operator[](iNextOpen); }
  void addReserve     ( unsigned int i ) { ensureCapacity(iNextOpen+i); }
  Array<T>& ensureCapacity ( unsigned int i ) {
    if ( i>=iCapacity ) {
      T* ptTmp = pt;
      iCapacity = (i<iCapacity*2) ? iCapacity*2 : i+1;
      pt=new T[iCapacity];
      for ( unsigned int j=0; j<iNextOpen; j++ )
        pt[j]=ptTmp[j];
      delete[] ptTmp;
    }
    return *this;
  }

  // Iterator methods...
  iterator        begin ( )       { return       iterator(this,0); }
  const_iterator& begin ( ) const { return const_iterator(this,0); }
  iterator        end   ( )       { return       iterator(this,iNextOpen); }
  const_iterator& end   ( ) const { return const_iterator(this,iNextOpen); }

  // Indexing methods...
  const T& operator[](unsigned int i) const { return get(i); }
  T&       operator[](unsigned int i)       { return set(i); }
  const T& get(unsigned int i) const { if (i>=iCapacity) return tDummy; /*assert(i<iCapacity);*/ return pt[i]; }
  T&       set(unsigned int i)       { ensureCapacity(i); if(i>=iNextOpen)iNextOpen=i+1; return pt[i]; }

  // Aliasing methods...
  T*       c_array ( )       { return pt; }  // NOTE: now it's risky!
  const T* c_array ( ) const { return pt; }

  // Extraction methods...
  unsigned int size()    const { return iNextOpen; }
  unsigned int getSize() const { return iNextOpen; }
  bool operator== ( const Array<T>& a ) const {
    if ( size() != a.size() ) return false;
    for ( unsigned int i=0; i<a.size(); i++ )
      if ( get(i) != a.get(i) ) return false;
    return true;
  }
  bool operator!= ( const Array<T>& a ) const { return !(*this==a); }

  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const Array<T>& at ) { for(unsigned int i=0;i<at.getSize();i++) os<<at.get(i)<<"\n"; return os; }
};

template<class T> const T Array<T>::tDummy = T();


////////////////////////////////////////////////////////////////////////////////

template<class T>
class SubArray {
 private:
  T*           pat;
  unsigned int length;
 public:
  // Constructor / destructor methods...
  SubArray ( )                                                 : pat(NULL),       length(0)           { }
  SubArray ( Array<T>& at )                                    : pat(&at.set(0)), length(at.size())   { }
  SubArray ( Array<T>& at,    unsigned int i )                 : pat(&at.set(i)), length(at.size()-i) { }
  SubArray ( Array<T>& at,    unsigned int i, unsigned int j ) : pat(&at.set(i)), length(j-i)         { }
  SubArray ( SubArray<T>& at, unsigned int i )                 : pat(&at.set(i)), length(at.size()-i) { }
  SubArray ( SubArray<T>& at, unsigned int i, unsigned int j ) : pat(&at.set(i)), length(j-i)         { }
  // Indexing methods...
  const T& operator[] ( unsigned int i ) const { return get(i); }
  T&       operator[] ( unsigned int i )       { return set(i); }
  const T& get        ( unsigned int i ) const { assert(i<length); return pat[i]; }
  T&       set        ( unsigned int i )       { assert(i<length); return pat[i]; }
  // Extraction methods...
  unsigned int size()    const { return length; }
  unsigned int getSize() const { return length; }
};


////////////////////////////////////////////////////////////////////////////////

#endif //_NL_ARRAY__

