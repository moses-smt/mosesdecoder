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

#ifndef __NL_HASH_H_
#define __NL_HASH_H_

#include <cassert>
//#include <tr1/unordered_map>
#include <ext/hash_map>
using namespace __gnu_cxx;


///////////////////////////////////////////////////////////////////////////////

template<class T>
class SimpleHashFn {
 public:
  size_t operator() ( const T& t ) const { return t.getHashKey(); }
};


template<class T>
class SimpleHashEqual {
 public:
  bool operator() ( const T& t1, const T& t2 ) const { return (t1 == t2); }
};


template<class X, class Y>
class SimpleHash : public hash_map<X,Y,SimpleHashFn<X>,SimpleHashEqual<X> > /*public tr1::unordered_map<X,Y,SimpleHashFn<X>,SimpleHashEqual<X> >*/ {
 private:
   typedef hash_map<X,Y,SimpleHashFn<X>,SimpleHashEqual<X> > OrigHash;
//  typedef tr1::unordered_map<X,Y,SimpleHashFn<X>,SimpleHashEqual<X> > OrigHash;
//  tr1::unordered_map<X,Y,SimpleHashFn<X>,SimpleHashEqual<X> > mxy;
  static const Y yDummy;
  //static Y yNonconstDummy;
  
 public:
//  typedef typename OrigHash::const_iterator const_iterator;
//  typedef typename OrigHash::iterator iterator;
//  static const const_iterator iDummy;
  // Constructor / destructor methods...
  SimpleHash ( )       : OrigHash()  { }
  SimpleHash ( int i ) : OrigHash(i) { }
  SimpleHash (const SimpleHash& s) : OrigHash(s) { }
  // Specification methods...
  Y&       set ( const X& x )       { return OrigHash::operator[](x); }
  // Extraction methods...
  const Y& get      ( const X& x ) const { return (OrigHash::end()!=OrigHash::find(x)) ? OrigHash::find(x)->second : yDummy; }
  bool     contains ( const X& x ) const { return (OrigHash::end()!=OrigHash::find(x)); }
//  const Y& get ( const X& x ) const { return (mxy.end()!=mxy.find(x)) ? mxy.find(x)->second : yDummy; }
//  Y&       set ( const X& x )       { return mxy[x]; }
  friend ostream& operator<< ( ostream& os, const SimpleHash<X,Y>& h ) {
    for ( typename SimpleHash<X,Y>::const_iterator it=h.begin(); it!=h.end(); it++ )
      os<<((it==h.begin())?"":",")<<it->first<<":"<<it->second;
    return os;
  }
};
template<class X, class Y> const Y SimpleHash<X,Y>::yDummy = Y();
//template<class X, class Y>  Y SimpleHash<X,Y>::yNonconstDummy; // = Y();

/*
template<class X, class Y>
class SimpleMultiHash : public tr1::unordered_multimap<X,Y,SimpleHashFn<X>,SimpleHashEqual<X> > {
 private:
  typedef tr1::unordered_multimap<X,Y,SimpleHashFn<X>,SimpleHashEqual<X> > OrigHash;
 public:
  typedef pair<typename OrigHash::const_iterator,typename OrigHash::const_iterator> const_iterator_pair;

  // Constructor / destructor methods...
  SimpleMultiHash ( )       : OrigHash()  { }
  SimpleMultiHash ( int i ) : OrigHash(i) { }
  // Specification methods...
  Y& add ( const X& x )  { return insert(typename OrigHash::value_type(x,Y()))->second; }
  // Extraction methods...
  bool contains ( const X& x )             const { return (OrigHash::end()!=OrigHash::find(x)); }
  bool contains ( const X& x, const Y& y ) const {
    if (OrigHash::end()==OrigHash::find(x)) return false;
    for ( const_iterator_pair ii=OrigHash::equal_range(x); ii.first!=ii.second; ii.first++ )
      if ( y == ii.first->second ) return true;
    return false;
  }
};
*/
#endif // __NL_HASH_H_
