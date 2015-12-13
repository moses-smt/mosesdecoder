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

#ifndef _NL_CPT__
#define _NL_CPT__

//#include <cstdlib>
//#include <vector>
//#include <string>
//#include <cassert>
//using namespace std;
//#include "nl-string.h"
//#include "nl-safeids.h"
//#include "nl-stringindex.h"
#include "nl-randvar.h"
//#include "nl-probmodel.h"
#include "nl-hash.h"
//#include <tr1/unordered_map>
//using namespace tr1;

#include <netinet/in.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  psNil
//
////////////////////////////////////////////////////////////////////////////////

char psNil[] = "";

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  Unit
//
////////////////////////////////////////////////////////////////////////////////

class Unit {
 public:
  void write(FILE* pf)const{}
  size_t getHashKey ( ) const { return 0; }
  bool operator== ( const Unit& u ) const { return true; }
  bool operator<  ( const Unit& u ) const { return false;  }
  friend ostream& operator<< ( ostream& os, const Unit& u ) { return os;  }
  friend String&  operator<< ( String& str, const Unit& u ) { return str; }
  friend IStream operator>> ( pair<IStream,Unit*> si_m, const char* psD ) { return si_m.first; }

  // OBSOLETE!
  friend pair<StringInput,Unit*> operator>> ( StringInput si, Unit& m ) { return pair<StringInput,Unit*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,Unit*> si_m, const char* psD ) { return si_m.first; }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  MapKeyXD
//
////////////////////////////////////////////////////////////////////////////////

////////////////////
template<class X1>
class MapKey1D {
 private:
  X1 x1;
 public:
  const X1& getX1() const { return x1; }
  size_t getHashKey ( ) const { return x1.getHashKey(); }
  MapKey1D ( ) { }
  MapKey1D ( const X1& a1 ) { x1=a1; }
  bool operator== ( const MapKey1D<X1>& h ) const { return(x1==h.x1); }
  bool operator<  ( const MapKey1D<X1>& h ) const { return(x1< h.x1); }
  friend ostream& operator<< ( ostream& os, const MapKey1D<X1>& k ) { return  os<<k.x1; }
  friend String&  operator<< ( String& str, const MapKey1D<X1>& k ) { return str<<k.x1; }
  friend IStream operator>> ( pair<IStream,MapKey1D<X1>*> is_m, const char* psD ) {
    //MapKey1D<X1>& m = *is_m.second;
    return is_m.first>>is_m.second->x1>>psD;
  }

  // OBSOLETE!
  friend pair<StringInput,MapKey1D<X1>*> operator>> ( StringInput si, MapKey1D<X1>& m ) {
    return pair<StringInput,MapKey1D<X1>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,MapKey1D<X1>*> si_m, const char* psD ) {
    MapKey1D<X1>& m = *si_m.second;
    return si_m.first>>m.x1>>psD;
  }
};

////////////////////
template<class X1, class X2>
class MapKey2D {
 private:
  X1 x1;
  X2 x2;
 public:
  const X1& getX1() const { return x1; }
  const X2& getX2() const { return x2; }
  size_t getHashKey ( ) const { size_t k=rotLeft(x1.getHashKey(),3); k^=x2.getHashKey(); return k; }
  MapKey2D ( ) { }
  MapKey2D ( const X1& a1, const X2& a2 ) { x1=a1; x2=a2; }
  bool operator== ( const MapKey2D<X1,X2>& h ) const { return(x1==h.x1 && x2==h.x2); }
  bool operator<  ( const MapKey2D<X1,X2>& h ) const {
    return ( (x1<h.x1) ||
             (x1==h.x1 && x2<h.x2) );
  }
  friend ostream& operator<< ( ostream& os, const MapKey2D<X1,X2>& k ) { return  os<<k.x1<<" "<<k.x2; }
  friend String&  operator<< ( String& str, const MapKey2D<X1,X2>& k ) { return str<<k.x1<<" "<<k.x2; }
  friend IStream operator>> ( pair<IStream,MapKey2D<X1,X2>*> is_m, const char* psD ) {
    MapKey2D<X1,X2>& m = *is_m.second; IStream is2;
    IStream is=is_m.first>>m.x1>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    return is>>m.x2>>psD;
  }

  // OBSOLETE!
  friend pair<StringInput,MapKey2D<X1,X2>*> operator>> ( StringInput si, MapKey2D<X1,X2>& m ) {
    return pair<StringInput,MapKey2D<X1,X2>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,MapKey2D<X1,X2>*> si_m, const char* psD ) {
    MapKey2D<X1,X2>& m = *si_m.second; StringInput si2;
    StringInput si=si_m.first>>m.x1>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    return si>>m.x2>>psD;
  }
};

////////////////////
template<class X1, class X2, class X3>
class MapKey3D {
 private:
  X1 x1;
  X2 x2;
  X3 x3;
 public:
  const X1& getX1() const { return x1; }
  const X2& getX2() const { return x2; }
  const X3& getX3() const { return x3; }
  size_t getHashKey ( ) const { size_t k=rotLeft(x1.getHashKey(),3); k=rotLeft(k^x2.getHashKey(),3); k^=x3.getHashKey(); return k; }
  MapKey3D ( ) { }
  MapKey3D ( const X1& a1, const X2& a2, const X3& a3 ) { x1=a1; x2=a2; x3=a3; }
  bool operator== ( const MapKey3D<X1,X2,X3>& h ) const { return(x1==h.x1 && x2==h.x2 && x3==h.x3); }
  bool operator<  ( const MapKey3D<X1,X2,X3>& h ) const {
    return ( (x1<h.x1) ||
             (x1==h.x1 && x2<h.x2) ||
             (x1==h.x1 && x2==h.x2 && x3<h.x3) );
  }
  friend ostream& operator<< ( ostream& os, const MapKey3D<X1,X2,X3>& k ) { return  os<<k.x1<<" "<<k.x2<<" "<<k.x3; }
  friend String&  operator<< ( String& str, const MapKey3D<X1,X2,X3>& k ) { return str<<k.x1<<" "<<k.x2<<" "<<k.x3; }
  friend IStream operator>> ( pair<IStream,MapKey3D<X1,X2,X3>*> is_m, const char* psD ) {
    MapKey3D<X1,X2,X3>& m = *is_m.second; IStream is2;
    IStream is=is_m.first>>m.x1>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x2>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    return is>>m.x3>>psD;
  }

  // OBSOLETE!
  friend pair<StringInput,MapKey3D<X1,X2,X3>*> operator>> ( StringInput si, MapKey3D<X1,X2,X3>& m ) {
    return pair<StringInput,MapKey3D<X1,X2,X3>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,MapKey3D<X1,X2,X3>*> si_m, const char* psD ) {
    MapKey3D<X1,X2,X3>& m = *si_m.second; StringInput si2;
    StringInput si=si_m.first>>m.x1>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x2>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    return si>>m.x3>>psD;
  }
};

////////////////////
template<class X1, class X2, class X3, class X4>
class MapKey4D {
 private:
  X1 x1;
  X2 x2;
  X3 x3;
  X4 x4;
 public:
  const X1& getX1() const { return x1; }
  const X2& getX2() const { return x2; }
  const X3& getX3() const { return x3; }
  const X4& getX4() const { return x4; }
  size_t getHashKey ( ) const { size_t k=rotLeft(x1.getHashKey(),3); k=rotLeft(k^x2.getHashKey(),3); k=rotLeft(k^x3.getHashKey(),3); k^=x4.getHashKey(); return k; }
  MapKey4D ( ) { }
  MapKey4D ( const X1& a1, const X2& a2, const X3& a3, const X4& a4 ) { x1=a1; x2=a2; x3=a3; x4=a4; }
  bool operator== ( const MapKey4D<X1,X2,X3,X4>& h ) const { return(x1==h.x1 && x2==h.x2 && x3==h.x3 && x4==h.x4); }
  bool operator<  ( const MapKey4D<X1,X2,X3,X4>& h ) const {
    return ( (x1<h.x1) ||
             (x1==h.x1 && x2<h.x2) ||
             (x1==h.x1 && x2==h.x2 && x3<h.x3) ||
             (x1==h.x1 && x2==h.x2 && x3==h.x3 && x4<h.x4) );
  }
  friend ostream& operator<< ( ostream& os, const MapKey4D<X1,X2,X3,X4>& k ) { return  os<<k.x1<<" "<<k.x2<<" "<<k.x3<<" "<<k.x4; }
  friend String&  operator<< ( String& str, const MapKey4D<X1,X2,X3,X4>& k ) { return str<<k.x1<<" "<<k.x2<<" "<<k.x3<<" "<<k.x4; }
  friend IStream operator>> ( pair<IStream,MapKey4D<X1,X2,X3,X4>*> is_m, const char* psD ) {
    MapKey4D<X1,X2,X3,X4>& m = *is_m.second; IStream is2;
    IStream is=is_m.first>>m.x1>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x2>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x3>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    return is>>m.x4>>psD;
  }

  // OBSOLETE!
  friend pair<StringInput,MapKey4D<X1,X2,X3,X4>*> operator>> ( StringInput si, MapKey4D<X1,X2,X3,X4>& m ) {
    return pair<StringInput,MapKey4D<X1,X2,X3,X4>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,MapKey4D<X1,X2,X3,X4>*> si_m, const char* psD ) {
    MapKey4D<X1,X2,X3,X4>& m = *si_m.second; StringInput si2;
    StringInput si=si_m.first>>m.x1>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x2>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x3>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    return si>>m.x4>>psD;
  }
};

////////////////////
template<class X1, class X2, class X3, class X4, class X5>
class MapKey5D {
 private:
  X1 x1;
  X2 x2;
  X3 x3;
  X4 x4;
  X5 x5;
 public:
  const X1& getX1() const { return x1; }
  const X2& getX2() const { return x2; }
  const X3& getX3() const { return x3; }
  const X4& getX4() const { return x4; }
  const X5& getX5() const { return x5; }
  size_t getHashKey ( ) const { size_t k=rotLeft(x1.getHashKey(),3); k=rotLeft(k^x2.getHashKey(),3);
                                       k=rotLeft(k^x3.getHashKey(),3); k=rotLeft(k^x4.getHashKey(),3); k^=x5.getHashKey(); return k; }
  MapKey5D ( ) { }
  MapKey5D ( const X1& a1, const X2& a2, const X3& a3, const X4& a4, const X5& a5 ) { x1=a1; x2=a2; x3=a3; x4=a4; x5=a5; }
  bool operator== ( const MapKey5D<X1,X2,X3,X4,X5>& h ) const { return(x1==h.x1 && x2==h.x2 && x3==h.x3 && x4==h.x4 && x5==h.x5); }
  bool operator<  ( const MapKey5D<X1,X2,X3,X4,X5>& h ) const {
    return ( (x1<h.x1) ||
             (x1==h.x1 && x2<h.x2) ||
             (x1==h.x1 && x2==h.x2 && x3<h.x3) ||
             (x1==h.x1 && x2==h.x2 && x3==h.x3 && x4<h.x4) ||
             (x1==h.x1 && x2==h.x2 && x3==h.x3 && x4==h.x4 && x5<h.x5) );
  }
  friend ostream& operator<< ( ostream& os, const MapKey5D<X1,X2,X3,X4,X5>& k ) { return  os<<k.x1<<" "<<k.x2<<" "<<k.x3<<" "<<k.x4<<" "<<k.x5; }
  friend String&  operator<< ( String& str, const MapKey5D<X1,X2,X3,X4,X5>& k ) { return str<<k.x1<<" "<<k.x2<<" "<<k.x3<<" "<<k.x4<<" "<<k.x5; }
  friend IStream operator>> ( pair<IStream,MapKey5D<X1,X2,X3,X4,X5>*> is_m, const char* psD ) {
    MapKey5D<X1,X2,X3,X4,X5>& m = *is_m.second; IStream is2;
    IStream is=is_m.first>>m.x1>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x2>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x3>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x4>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    return is>>m.x5>>psD;
  }

  // OBSOLETE!
  friend pair<StringInput,MapKey5D<X1,X2,X3,X4,X5>*> operator>> ( StringInput si, MapKey5D<X1,X2,X3,X4,X5>& m ) {
    return pair<StringInput,MapKey5D<X1,X2,X3,X4,X5>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,MapKey5D<X1,X2,X3,X4,X5>*> si_m, const char* psD ) {
    MapKey5D<X1,X2,X3,X4,X5>& m = *si_m.second; StringInput si2;
    StringInput si=si_m.first>>m.x1>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x2>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x3>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x4>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    return si>>m.x5>>psD;
  }
};

////////////////////
template<class X1, class X2, class X3, class X4, class X5, class X6>
class MapKey6D {
 private:
  X1 x1;
  X2 x2;
  X3 x3;
  X4 x4;
  X5 x5;
  X6 x6;
 public:
  const X1& getX1() const { return x1; }
  const X2& getX2() const { return x2; }
  const X3& getX3() const { return x3; }
  const X4& getX4() const { return x4; }
  const X5& getX5() const { return x5; }
  const X5& getX6() const { return x6; }
  size_t getHashKey ( ) const { size_t k=rotLeft(x1.getHashKey(),3); k=rotLeft(k^x2.getHashKey(),3);
                                       k=rotLeft(k^x3.getHashKey(),3); k=rotLeft(k^x4.getHashKey(),3); k=rotLeft(k^x5.getHashKey(),3); k^=x6.getHashKey(); return k; }
  MapKey6D ( ) { }
  MapKey6D ( const X1& a1, const X2& a2, const X3& a3, const X4& a4, const X5& a5, const X6& a6 ) { x1=a1; x2=a2; x3=a3; x4=a4; x5=a5; x6=a6; }
  bool operator== ( const MapKey6D<X1,X2,X3,X4,X5,X6>& h ) const { return(x1==h.x1 && x2==h.x2 && x3==h.x3 && x4==h.x4 && x5==h.x5 && x6==h.x6); }
  bool operator<  ( const MapKey6D<X1,X2,X3,X4,X5,X6>& h ) const {
    return ( (x1<h.x1) ||
             (x1==h.x1 && x2<h.x2) ||
             (x1==h.x1 && x2==h.x2 && x3<h.x3) ||
             (x1==h.x1 && x2==h.x2 && x3==h.x3 && x4<h.x4) ||
             (x1==h.x1 && x2==h.x2 && x3==h.x3 && x4==h.x4 && x5<h.x5) ||
             (x1==h.x1 && x2==h.x2 && x3==h.x3 && x4==h.x4 && x5==h.x5 && x6<h.x6) );
  }
  friend ostream& operator<< ( ostream& os, const MapKey6D<X1,X2,X3,X4,X5,X6>& k ) { return  os<<k.x1<<" "<<k.x2<<" "<<k.x3<<" "<<k.x4<<" "<<k.x5<<" "<<k.x6; }
  friend String&  operator<< ( String& str, const MapKey6D<X1,X2,X3,X4,X5,X6>& k ) { return str<<k.x1<<" "<<k.x2<<" "<<k.x3<<" "<<k.x4<<" "<<k.x5<<" "<<k.x6; }
  friend IStream operator>> ( pair<IStream,MapKey6D<X1,X2,X3,X4,X5,X6>*> is_m, const char* psD ) {
    MapKey6D<X1,X2,X3,X4,X5,X6>& m = *is_m.second; IStream is2;
    IStream is=is_m.first>>m.x1>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x2>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x3>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x4>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    is=is>>m.x5>>" ";
    while((is2=is>>" ")!=IStream())is=is2;
    return is>>m.x6>>psD;
  }

  // OBSOLETE!
  friend pair<StringInput,MapKey6D<X1,X2,X3,X4,X5,X6>*> operator>> ( StringInput si, MapKey6D<X1,X2,X3,X4,X5,X6>& m ) {
    return pair<StringInput,MapKey6D<X1,X2,X3,X4,X5,X6>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,MapKey6D<X1,X2,X3,X4,X5,X6>*> si_m, const char* psD ) {
    MapKey6D<X1,X2,X3,X4,X5,X6>& m = *si_m.second; StringInput si2;
    StringInput si=si_m.first>>m.x1>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x2>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x3>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x4>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>m.x5>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    return si>>m.x6>>psD;
  }
};

// Declare random access conditional probability tables (un-iteratable)
#include "nl-racpt.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  Multimap CPTXDModel
//
////////////////////////////////////////////////////////////////////////////////

/*
template<class II,class V>
class BaseIterVal : public V {
 public:
  static const int NUM_ITERS = 1;
  II iter;
  BaseIterVal ( )       : V(), iter(typename II::first_type(0,0),typename II::second_type(0,0))   { }
  BaseIterVal ( II ii ) : V((ii.first!=ii.second)?ii.first->first:V()), iter(ii)                  { }
  BaseIterVal ( V v )   : V(v), iter(typename II::first_type(0,0),typename II::second_type(0,0))  { }
  BaseIterVal& operator++ ( int ) { V& v=*this; iter.first++; if(iter.first!=iter.second)v=iter.first->first; return *this; }
};
*/

////////////////////////////////////////////////////////////////////////////////


template<class X, class Y>
class SimpleMap : public map<X,Y> {
 private:
  typedef map<X,Y> OrigMap;
  static const Y yDummy;

 public:
  // Constructor / destructor methods...
  SimpleMap ( )       : OrigMap() { }
  SimpleMap ( int i ) : OrigMap() { }
  SimpleMap (const SimpleMap& s) : OrigMap(s) { }
  // Specification methods...
  Y&       set ( const X& x )       { return OrigMap::operator[](x); }
  // Extraction methods...
  const Y& get      ( const X& x ) const { return (OrigMap::end()!=OrigMap::find(x)) ? OrigMap::find(x)->second : yDummy; }
  bool     contains ( const X& x ) const { return (OrigMap::end()!=OrigMap::find(x)); }
  friend ostream& operator<< ( ostream& os, const SimpleMap<X,Y>& h ) {
    for ( typename SimpleMap<X,Y>::const_iterator it=h.begin(); it!=h.end(); it++ )
      os<<((it==h.begin())?"":",")<<it->first<<":"<<it->second;
    return os;
  }
};
template<class X, class Y> const Y SimpleMap<X,Y>::yDummy = Y();


////////////////////////////////////////////////////////////////////////////////

#define MAP_CONTAINER SimpleMap
//#define MAP_CONTAINER SimpleHash

template<class Y, class K, class P, const char* psLbl=psNil>
class GenericCPTModel : public MAP_CONTAINER<K,MAP_CONTAINER<Y,P> > {
 private:
  typedef MAP_CONTAINER<K,MAP_CONTAINER<Y,P> > HKYP;
  typedef typename MAP_CONTAINER<Y,P>::const_iterator IYP;
  //HKYP h;

 public:
  typedef Y RandVarType;
  typedef Y RVType;
/*   typedef BaseIterVal<std::pair<IYP,IYP>,Y> IterVal; */
  typedef MAP_CONTAINER<Y,P> distribution;

/*   bool setFirst ( IterVal& ikyp, const K& k ) const { */
/*     const MAP_CONTAINER<Y,P>& hyp = MAP_CONTAINER<K,MAP_CONTAINER<Y,P> >::get(k); */
/*     ikyp = std::pair<IYP,IYP>(hyp.begin(),hyp.end()); */
/*     return ( ikyp.iter.first != ikyp.iter.second ); */
/*   } */
/*   bool setNext ( IterVal& ikyp, const K& k ) const { */
/*     if ( ikyp.iter.first != ikyp.iter.second ) ikyp++; */
/*     return ( ikyp.iter.first != ikyp.iter.second ); */
/*   } */
  bool contains ( const Y& y, const K& k ) const {
    return ( MAP_CONTAINER<K,MAP_CONTAINER<Y,P> >::get(k).contains(y) );
  }
  bool contains ( const K& k ) const {
    return ( MAP_CONTAINER<K,MAP_CONTAINER<Y,P> >::contains(k) );
  }
/*   P getProb ( const IterVal& ikyp, const K& k ) const { */
/*     if ( ikyp.iter.first == ikyp.iter.second ) { cerr<<"ERROR: no iterator to fix probability: "<<k<<endl; return P(); } */
/*     return ( ikyp.iter.first->second ); */
/*   } */
  P getProb ( const Y& y, const K& k ) const {
    return MAP_CONTAINER<K,MAP_CONTAINER<Y,P> >::get(k).get(y);
  }
  const MAP_CONTAINER<Y,P>& getDist ( const K& k ) const {
    return MAP_CONTAINER<K,MAP_CONTAINER<Y,P> >::get(k);
  }
  P& setProb ( const Y& y, const K& k ) {
    return MAP_CONTAINER<K,MAP_CONTAINER<Y,P> >::set(k).set(y);
  }
  void normalize ( ) {
    for ( typename HKYP::iterator ik=HKYP::begin(); ik!=HKYP::end(); ik++ ) {
      K k=ik->first;
      P p=P();
      for ( typename distribution::iterator itd = ik->second.begin(); itd != ik->second.end(); ++itd )
        p += itd->second;
      if (p!=P())
        for ( typename distribution::iterator itd = ik->second.begin(); itd != ik->second.end(); ++itd )
          itd->second /= p;
    }
  }
/*   void transmit ( int tSockfd, const char* psId ) const { */
/*     for ( typename HKYP::const_iterator ik=HKYP::begin(); ik!=HKYP::end(); ik++ ) { */
/*       K k=ik->first; */
/*       IterVal y; */
/*       // For each non-zero probability in model... */
/*       for ( bool b=setFirst(y,k); b; b=setNext(y,k) ) { */
/*         //if ( getProb(y,k) != P() ) { */
/*           String str(1000); */
/*           str<<psId<<" "<<k<<" : "<<y<<" = "<<getProb(y,k).toDouble()<<"\n"; */
/*           if ( send(tSockfd,str.c_array(),str.size()-1,MSG_EOR) != int(str.size()-1) ) */
/*             {cerr<<"ERROR writing to socket\n";exit(0);} */
/*         //} */
/*       } */
/*     } */
/*   } */
  void dump ( ostream& os, const char* psId ) const {
    for ( typename HKYP::const_iterator ik=HKYP::begin(); ik!=HKYP::end(); ik++ ) {
      K k=ik->first;
      const distribution& dist = ik->second;
      for ( typename distribution::const_iterator itd = dist.begin(); itd != dist.end(); ++itd ) {
        const Y& y = itd->first;
        os<<psId<<" "<<k<<" : "<<y<<" = "<<getProb(y,k).toDouble()<<"\n";
      }
    }
  }
/*   void subsume ( GenericCPTModel<Y,K,P>& m ) { */
/*     for ( typename HKYP::const_iterator ik=m.HKYP::begin(); ik!=m.HKYP::end(); ik++ ) { */
/*       K k=ik->first; */
/*       IterVal y; */
/*       for ( bool b=m.setFirst(y,k); b; b=m.setNext(y,k) ) */
/*         setProb(y,k) = m.getProb(y,k); */
/*     } */
/*   } */
  void clear ( ) { MAP_CONTAINER<K,MAP_CONTAINER<Y,P> >::clear(); }

  //friend pair<IStream,GenericCPTModel<Y,K,P>*> operator>> ( IStream is, GenericCPTModel<Y,K,P>& m ) {
  //  return pair<IStream,GenericCPTModel<Y,K,P>*>(is,&m); }
  friend IStream operator>> ( pair<IStream,GenericCPTModel<Y,K,P,psLbl>*> is_m, const char* psD ) {
    Y y; K k; IStream is,is1; GenericCPTModel<Y,K,P,psLbl>& m = *is_m.second;
    is=is_m.first;
    if ( is==IStream() ) return is;
    is=is>>psLbl;
    while((is1=is>>" ")!=IStream())is=is1;
    ////cerr<<"reading k...\n";
    is=is>>k>>" ";
    ////cerr<<"         ...k='"<<k<<"'\n";
    while((is1=is>>" ")!=IStream())is=is1;
    is=is>>": ";
    while((is1=is>>" ")!=IStream())is=is1;
    ////cerr<<"reading y...\n";
    is=is>>y>>" ";
    ////cerr<<"         ...y='"<<y<<"'\n";
    while((is1=is>>" ")!=IStream())is=is1;
    is=is>>"= ";
    while((is1=is>>" ")!=IStream())is=is1;
    ////cerr<<"reading pr...\n";
    return (is!=IStream()) ? is>>m.setProb(y,k)>>psD : is;
  }

  // OBSOLETE!
  friend pair<StringInput,GenericCPTModel<Y,K,P,psLbl>*> operator>> ( StringInput si, GenericCPTModel<Y,K,P,psLbl>& m ) {
    return pair<StringInput,GenericCPTModel<Y,K,P,psLbl>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,GenericCPTModel<Y,K,P,psLbl>*> si_m, const char* psD ) {
    Y y; K k; StringInput si,si2; GenericCPTModel<Y,K,P,psLbl>& m = *si_m.second;
    si=si_m.first;
    if ( si==NULL ) return si;
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>k>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>": ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>y>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>"= ";
    while((si2=si>>" ")!=NULL)si=si2;
    return (si!=NULL) ? si>>m.setProb(y,k)>>psD : si;
  }
};


////////////////////////////////////////////////////////////////////////////////

////////////////////
template<class Y, class P, const char* psLbl=psNil>
class CPT1DModel : public GenericCPTModel<Y,MapKey1D<Unit>,P,psLbl> {
  typedef GenericCPTModel<Y,MapKey1D<Unit>,P,psLbl> ParentType;
  typedef CPT1DModel<Y,P,psLbl> ThisType;
 public:
/*   typedef typename GenericCPTModel<Y,MapKey1D<Unit>,P>::IterVal IterVal; */

/*   bool setFirst ( IterVal& ixyp ) const { */
/*     return GenericCPTModel<Y,MapKey1D<Unit>,P>::setFirst ( ixyp, MapKey1D<Unit>(Unit()) ); */
/*   } */
/*   bool setNext ( IterVal& ixyp ) const { */
/*     return GenericCPTModel<Y,MapKey1D<Unit>,P>::setNext ( ixyp, MapKey1D<Unit>(Unit()) ); */
/*   } */
  bool contains ( const Y& y ) const {
    return GenericCPTModel<Y,MapKey1D<Unit>,P,psLbl>::contains ( y, MapKey1D<Unit>(Unit()) );
  }
/*   P getProb ( const IterVal& ixyp ) const { */
/*     return GenericCPTModel<Y,MapKey1D<Unit>,P>::getProb ( ixyp, MapKey1D<Unit>(Unit()) ); */
/*   } */
  P getProb ( const Y& y ) const {
    return GenericCPTModel<Y,MapKey1D<Unit>,P,psLbl>::getProb ( y, MapKey1D<Unit>(Unit()) );
  }
  const typename GenericCPTModel<Y,MapKey1D<Unit>,P,psLbl>::distribution& getDist ( ) const {
    return GenericCPTModel<Y,MapKey1D<Unit>,P,psLbl>::get ( MapKey1D<Unit>(Unit()) );
  }
  P& setProb ( const Y& y ) {
    return GenericCPTModel<Y,MapKey1D<Unit>,P,psLbl>::setProb ( y, MapKey1D<Unit>(Unit()) );
  }
  bool readFields ( Array<char*>& aps ) {
    if ( 3==aps.size() ) {
      GenericCPTModel<Y,MapKey1D<Unit>,P,psLbl>::setProb ( Y(aps[1]), MapKey1D<Unit>(Unit()) ) = atof(aps[2]);
      return true;
    }
    return false;
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};


////////////////////
template<class Y, class X1, class P, const char* psLbl=psNil>
class CPT2DModel : public GenericCPTModel<Y,MapKey1D<X1>,P,psLbl> {
  typedef GenericCPTModel<Y,MapKey1D<X1>,P,psLbl> ParentType;
  typedef CPT2DModel<Y,X1,P,psLbl> ThisType;
 public:
/*   typedef typename GenericCPTModel<Y,MapKey1D<X1>,P>::IterVal IterVal; */

/*   // This stuff only for deterministic 'Determ' models... */
/*   typedef X1 Dep1Type; */
/*   typedef P ProbType; */
/*   bool hasDeterm ( const X1& x1 ) { IterVal y; bool b=setFirst(y,x1); return b; } */
/*   Y    getDeterm ( const X1& x1 ) { IterVal y; bool b=setFirst(y,x1); if(!b)cerr<<"ERROR: determ case missing: "<<x1.getString()<<endl; return y; } */

/*   bool setFirst ( IterVal& ixyp, const X1& x1 ) const { */
/*     return GenericCPTModel<Y,MapKey1D<X1>,P>::setFirst ( ixyp, MapKey1D<X1>(x1) ); */
/*   } */
/*   bool setNext ( IterVal& ixyp, const X1& x1 ) const { */
/*     return GenericCPTModel<Y,MapKey1D<X1>,P>::setNext ( ixyp, MapKey1D<X1>(x1) ); */
/*   } */
  bool contains ( const Y& y, const X1& x1 ) const {
    return GenericCPTModel<Y,MapKey1D<X1>,P,psLbl>::contains ( y, MapKey1D<X1>(x1) );
  }
  bool contains ( const X1& x1 ) const {
    return GenericCPTModel<Y,MapKey1D<X1>,P,psLbl>::contains ( MapKey1D<X1>(x1) );
  }
/*   P getProb ( const IterVal& ixyp, const X1& x1 ) const { */
/*     return GenericCPTModel<Y,MapKey1D<X1>,P>::getProb ( ixyp, MapKey1D<X1>(x1) ); */
/*   } */
  P getProb ( const Y& y, const X1& x1 ) const {
    return GenericCPTModel<Y,MapKey1D<X1>,P,psLbl>::getProb ( y, MapKey1D<X1>(x1) );
  }
  const typename GenericCPTModel<Y,MapKey1D<X1>,P,psLbl>::distribution& getDist ( const X1& x1 ) const {
    return GenericCPTModel<Y,MapKey1D<X1>,P,psLbl>::get ( MapKey1D<X1>(x1) );
  }
  P& setProb ( const Y& y, const X1& x1 ) {
    return GenericCPTModel<Y,MapKey1D<X1>,P,psLbl>::setProb ( y, MapKey1D<X1>(x1) );
  }
  bool readFields ( Array<char*>& aps ) {
    if ( 4==aps.size() ) {
      GenericCPTModel<Y,MapKey1D<X1>,P,psLbl>::setProb ( Y(aps[2]), MapKey1D<X1>(aps[1]) ) = atof(aps[3]);
      return true;
    }
    return false;
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};


////////////////////
template<class Y, class X1, class X2, class P, const char* psLbl=psNil>
class CPT3DModel : public GenericCPTModel<Y,MapKey2D<X1,X2>,P,psLbl> {
  typedef GenericCPTModel<Y,MapKey2D<X1,X2>,P,psLbl> ParentType;
  typedef CPT3DModel<Y,X1,X2,P,psLbl> ThisType;
 public:
/*   typedef typename GenericCPTModel<Y,MapKey2D<X1,X2>,P>::IterVal IterVal; */

/*   bool setFirst ( IterVal& ixyp, const X1& x1, const X2& x2 ) const { */
/*     return GenericCPTModel<Y,MapKey2D<X1,X2>,P>::setFirst ( ixyp, MapKey2D<X1,X2>(x1,x2) ); */
/*   } */
/*   bool setNext ( IterVal& ixyp, const X1& x1, const X2& x2 ) const { */
/*     return GenericCPTModel<Y,MapKey2D<X1,X2>,P>::setNext ( ixyp, MapKey2D<X1,X2>(x1,x2) ); */
/*   } */
  bool contains ( const Y& y, const X1& x1, const X2& x2 ) const {
    return GenericCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::contains ( y, MapKey2D<X1,X2>(x1,x2) );
  }
  bool contains ( const X1& x1, const X2& x2 ) const {
    return GenericCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::contains ( MapKey2D<X1,X2>(x1,x2) );
  }
/*   P getProb ( const IterVal& ixyp, const X1& x1, const X2& x2 ) const { */
/*     return GenericCPTModel<Y,MapKey2D<X1,X2>,P>::getProb ( ixyp, MapKey2D<X1,X2>(x1,x2) ); */
/*   } */
  P getProb ( const Y& y, const X1& x1, const X2& x2 ) const {
    return GenericCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::getProb ( y, MapKey2D<X1,X2>(x1,x2) );
  }
  const typename GenericCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::distribution& getDist ( const X1& x1, const X2& x2 ) const {
    return GenericCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::get ( MapKey2D<X1,X2>(x1,x2) );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2 ) {
    return GenericCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::setProb ( y, MapKey2D<X1,X2>(x1,x2) );
  }
  bool readFields ( Array<char*>& aps ) {
    if ( 5==aps.size() ) {
      GenericCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::setProb ( Y(aps[3]), MapKey2D<X1,X2>(aps[1],aps[2]) ) = atof(aps[4]);
      return true;
    }
    return false;
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};


////////////////////
template<class Y, class X1, class X2, class X3, class P, const char* psLbl=psNil>
class CPT4DModel : public GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl> {
  typedef GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl> ParentType;
  typedef CPT4DModel<Y,X1,X2,X3,P,psLbl> ThisType;
 public:
/*   typedef typename GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::IterVal IterVal; */

/*   bool setFirst ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3 ) const { */
/*     return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::setFirst ( ixyp, MapKey3D<X1,X2,X3>(x1,x2,x3) ); */
/*   } */
/*   bool setNext ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3 ) const { */
/*     return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::setNext ( ixyp, MapKey3D<X1,X2,X3>(x1,x2,x3) ); */
/*   } */
  bool contains ( const Y& y, const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::contains ( y, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::contains ( MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
/*   P getProb ( const IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3 ) const { */
/*     return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::getProb ( ixyp, MapKey3D<X1,X2,X3>(x1,x2,x3) ); */
/*   } */
  P getProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::getProb ( y, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  const typename GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::distribution& getDist ( const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::get ( MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3 ) {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::setProb ( y, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  bool readFields ( Array<char*>& aps ) {
    if ( 6==aps.size() ) {
      GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::setProb ( Y(aps[4]), MapKey3D<X1,X2,X3>(aps[1],aps[2],aps[3]) ) = atof(aps[5]);
      return true;
    }
    return false;
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};


////////////////////
template<class Y, class X1, class X2, class X3, class X4, class P, const char* psLbl=psNil>
class CPT5DModel : public GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl> {
  typedef GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl> ParentType;
  typedef CPT5DModel<Y,X1,X2,X3,X4,P,psLbl> ThisType;
 public:
/*   typedef typename GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::IterVal IterVal; */

/*   bool setFirst ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const { */
/*     return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::setFirst ( ixyp, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) ); */
/*   } */
/*   bool setNext ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const { */
/*     return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::setNext ( ixyp, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) ); */
/*   } */
  bool contains ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::contains ( y, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::contains ( MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
/*   P getProb ( const IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const { */
/*     return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::getProb ( ixyp, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) ); */
/*   } */
  P getProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::getProb ( y, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  const typename GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::distribution& getDist ( const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::get ( MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::setProb ( y, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  bool readFields ( Array<char*>& aps ) {
    if ( 7==aps.size() ) {
      GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::setProb ( Y(aps[5]), MapKey4D<X1,X2,X3,X4>(aps[1],aps[2],aps[3],aps[4]) ) = atof(aps[6]);
      return true;
    }
    return false;
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};


////////////////////
template<class Y, class X1, class X2, class X3, class X4, class X5, class P, const char* psLbl=psNil>
class CPT6DModel : public GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl> {
  typedef GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl> ParentType;
  typedef CPT6DModel<Y,X1,X2,X3,X4,X5,P,psLbl> ThisType;
 public:
/*   typedef typename GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::IterVal IterVal; */

/*   bool setFirst ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const { */
/*     return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::setFirst ( ixyp, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) ); */
/*   } */
/*   bool setNext ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const { */
/*     return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::setNext ( ixyp, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) ); */
/*   } */
  bool contains ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::contains ( y, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::contains ( MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
/*   P getProb ( const IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const { */
/*     return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::getProb ( ixyp, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) ); */
/*   } */
  P getProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::getProb ( y, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  const typename GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::distribution& getDist ( const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::get ( MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::setProb ( y, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  bool readFields ( Array<char*>& aps ) {
    if ( 8==aps.size() ) {
      GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::setProb ( Y(aps[6]), MapKey5D<X1,X2,X3,X4,X5>(aps[1],aps[2],aps[3],aps[4],aps[5]) ) = atof(aps[7]);
      return true;
    }
    return false;
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};













///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
template<class Y, class P>
class HidVarBaseIterVal : public pair<SafePtr<const Array<pair<Y,P> > >,Id<int> > {
 public:
  typedef Y VAR;
  static const int NUM_ITERS = 1;
  operator Y ( ) const { return pair<SafePtr<const Array<pair<Y,P> > >,Id<int> >::first.getRef().get(pair<SafePtr<const Array<pair<Y,P> > >,Id<int> >::second.toInt()).first; }
};
*/

template<class Y, class K, class P, const char* psLbl=psNil>
//class GenericHidVarCPTModel : public SimpleHash<K,Array<pair<Y,P> > > {
class GenericHidVarCPTModel : public SimpleHash<K,typename Y::template ArrayDistrib<P> > {
 private:
  //typedef SimpleHash<K,typename Y::template Distrib<P> > HKYP;
  //typedef SimpleHash<K,Array<pair<Y,P> > > HKYP;
  typedef SimpleHash<K,typename Y::template ArrayDistrib<P> > HKYP;
  typedef int IYP;
  ////HKYP h;

 public:
  typedef Y RandVarType;
  typedef Y RVType;
  typedef typename Y::template ArrayIterator<P> IterVal;

  typename HKYP::const_iterator begin ( ) const { return HKYP::begin(); }
  typename HKYP::iterator       begin ( )       { return HKYP::begin(); }
  typename Y::template ArrayIterator<P> begin ( const K& k ) const {
  typename Y::template ArrayIterator<P> iyp; iyp.first = HKYP::get(k); iyp.second = 0; return iyp;
  }
  P setIterProb ( typename Y::template ArrayIterator<P>& iyp, const K& k, int& vctr ) const {
    P pr;
    // If fork happens before this var, set val to first in distrib and return prob=1.0...
    if ( vctr < 0 ) {
      iyp.first = HKYP::get(k);
      iyp.second = 0;
      pr = ( iyp.first.getRef().get(0).second != P() ) ? P(1.0) : P();
      if (pr == P()){
        cerr << "ERROR: Some condition has no value!  Key = '" << k << "' vctr=" << vctr << "\n";
      }
      //return P(1.0);
    }
    // If fork happens at this var, set val to next in distrib and return prob of val (prob=0.0 if nothing is next)...
    // NOTE: falling off the end of a distribution is not a model error; only failing to start a new distribution!
    else if ( vctr == 0 ) {
      if (iyp.second+1<iyp.first.getRef().getSize()) { iyp.second=iyp.second+1; pr = iyp.first.getRef().get(iyp.second.toInt()).second; }
      else pr = P();
    }
    // If fork happens after this var, retain val and return prob of val...
    else {
      pr = iyp.first.getRef().get(iyp.second.toInt()).second;
    }
    vctr--;
    return pr;
  }
  bool contains ( const Y& y, const K& k ) const {
    cerr<<"ERROR: GenericHidVarCPTModel no support for 'contains'!\n";
    return false;
  }
  bool contains ( const K& k ) const {
    return ( HKYP::contains(k) );
  }
  P getProb ( const typename Y::template ArrayIterator<P>& iyp, const K& k ) const {
    return iyp.first.getRef().get(iyp.second.toInt()).second;
  }
  const typename Y::template ArrayDistrib<P>& getDistrib ( const K& k ) const {
    return HKYP::get(k);
  }

  P& setProb ( const Y& y, const K& k ) {
    pair<typename Y::BaseType,P>& yp = HKYP::set(k).add();
    yp.first = y;
    return yp.second;
  }
  typename Y::template ArrayDistrib<P>& setDistrib ( const K& k ) {
    return HKYP::set(k);
  }
  void normalize ( ) {
    // NOTE: BEAR IN MIND, LOGPROBS CAN'T BE ADDED!!!!!
    for ( typename HKYP::iterator ik=HKYP::begin(); ik!=HKYP::end(); ik++ ) {
      P prTot;
      for ( unsigned int i=0; i<ik->second.size(); i++ ) {
	prTot += ik->second.get(i).second;
      }
      for ( unsigned int i=0; i<ik->second.size(); i++ ) {
	ik->second.set(i).second /= prTot;
      }
    }
  }
  void dump ( ostream& os, const char* psId ) const {
    for ( typename HKYP::const_iterator ik=HKYP::begin(); ik!=HKYP::end(); ik++ ) {
      K k=ik->first;
      //IterVal y;
      //for ( bool b=setFirst(y,k); b; b=setNext(y,k) )
      for ( unsigned int i=0; i<ik->second.size(); i++ ) {
        const pair<Y,P>& yp = ik->second.get(i);
        os<<psId<<" "<<k<<" : "<<yp.first<<" = "<<yp.second.toDouble()<<"\n";
      }
    }
  }
  void clear ( ) { HKYP::clear(); }

//  friend pair<IStream,GenericHidVarCPTModel<Y,K,P,psLbl>*> operator>> ( IStream is, GenericHidVarCPTModel<Y,K,P,psLbl>& m ) {
//    return pair<IStream,GenericHidVarCPTModel<Y,K,P,psLbl>*>(is,&m); }
  friend IStream operator>> ( pair<IStream,GenericHidVarCPTModel<Y,K,P,psLbl>*> is_m, const char* psD ) {
    Y y; K k; IStream is,is1; GenericHidVarCPTModel<Y,K,P,psLbl>& m = *is_m.second;
    is=is_m.first;
    if ( is==IStream() ) return is;
    is=is>>psLbl;
    while((is1=is>>" ")!=IStream())is=is1;
    ////cerr<<"reading k...\n";
    is=is>>k>>" ";
    ////cerr<<"         ...k='"<<k<<"'\n";
    while((is1=is>>" ")!=IStream())is=is1;
    is=is>>": ";
    while((is1=is>>" ")!=IStream())is=is1;
    ////cerr<<"reading y...\n";
    is=is>>y>>" ";
    ////cerr<<"         ...y='"<<y<<"'\n";
    while((is1=is>>" ")!=IStream())is=is1;
    is=is>>"= ";
    while((is1=is>>" ")!=IStream())is=is1;
    ////cerr<<"reading pr...\n";
    return (is!=IStream()) ? is>>m.setProb(y,k)>>psD : is;
  }

  // OBSOLETE!
  friend pair<StringInput,GenericHidVarCPTModel<Y,K,P,psLbl>*> operator>> ( StringInput si, GenericHidVarCPTModel<Y,K,P,psLbl>& m ) {
    return pair<StringInput,GenericHidVarCPTModel<Y,K,P,psLbl>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,GenericHidVarCPTModel<Y,K,P,psLbl>*> si_m, const char* psD ) {
    Y y; K k; StringInput si,si2; GenericHidVarCPTModel<Y,K,P,psLbl>& m = *si_m.second;
    si=si_m.first;
    if ( si==NULL ) return si;
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>k>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>": ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>y>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>"= ";
    while((si2=si>>" ")!=NULL)si=si2;
    return (si!=NULL) ? si>>m.setProb(y,k)>>psD : si;
  }
};

////////////////////////////////////////////////////////////////////////////////

////////////////////
template<class Y, class P, const char* psLbl=psNil>
class HidVarCPT1DModel : public GenericHidVarCPTModel<Y,MapKey1D<Unit>,P,psLbl> {
  typedef GenericHidVarCPTModel<Y,MapKey1D<Unit>,P,psLbl> ParentType;
  typedef HidVarCPT1DModel<Y,P,psLbl> ThisType;
 public:
  HidVarCPT1DModel ( )            { }
  HidVarCPT1DModel ( const Y& y ) { setProb(y)=P(1.0); }
  P setIterProb ( typename Y::template ArrayIterator<P>& iyp, int& vctr ) const {
    return GenericHidVarCPTModel<Y,MapKey1D<Unit>,P,psLbl>::setIterProb ( iyp, MapKey1D<Unit>(), vctr );
  }
  P& setProb ( const Y& y ) {
    return GenericHidVarCPTModel<Y,MapKey1D<Unit>,P,psLbl>::setProb ( y, MapKey1D<Unit>() );
  }
  P getProb ( const typename Y::template ArrayIterator<P>& iyp) const {
    return GenericHidVarCPTModel<Y,MapKey1D<Unit>,P,psLbl>::getProb ( iyp, MapKey1D<Unit>() );
  }
  const typename Y::template ArrayDistrib<P>& getDistrib ( ) const {
    return GenericHidVarCPTModel<Y,MapKey1D<Unit>,P,psLbl>::getDistrib ( MapKey1D<Unit>() );
  }
  bool contains ( ) const {
    return GenericHidVarCPTModel<Y,MapKey1D<Unit>,P,psLbl>::contains(MapKey1D<Unit>());
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};

////////////////////
template<class Y, class X1, class P, const char* psLbl=psNil>
class HidVarCPT2DModel : public GenericHidVarCPTModel<Y,MapKey1D<X1>,P,psLbl> {
  typedef GenericHidVarCPTModel<Y,MapKey1D<X1>,P,psLbl> ParentType;
  typedef HidVarCPT2DModel<Y,X1,P,psLbl> ThisType;
 public:
  P setIterProb ( typename Y::template ArrayIterator<P>& iyp, const X1& x1, int& vctr ) const {
    return GenericHidVarCPTModel<Y,MapKey1D<X1>,P,psLbl>::setIterProb ( iyp, MapKey1D<X1>(x1), vctr );
  }
  P& setProb ( const Y& y, const X1& x1 ) {
    return GenericHidVarCPTModel<Y,MapKey1D<X1>,P,psLbl>::setProb ( y, MapKey1D<X1>(x1) );
  }
  typename Y::template ArrayDistrib<P>& setDistrib ( const X1& x1 ) {
    return GenericHidVarCPTModel<Y,MapKey1D<X1>,P,psLbl>::setDistrib ( MapKey1D<X1>(x1) );
  }
  P getProb ( const typename Y::template ArrayIterator<P>& iyp, const X1& x1) const {
      return GenericHidVarCPTModel<Y,MapKey1D<X1>,P,psLbl>::getProb ( iyp, MapKey1D<X1>(x1) );
  }
  const typename Y::template ArrayDistrib<P>& getDistrib ( const X1& x1 ) const {
    return GenericHidVarCPTModel<Y,MapKey1D<X1>,P,psLbl>::getDistrib ( MapKey1D<X1>(x1) );
  }
  bool contains ( const X1& x1 ) const {
    return GenericHidVarCPTModel<Y,MapKey1D<X1>,P,psLbl>::contains(MapKey1D<X1>(x1));
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};

////////////////////
template<class Y, class X1, class X2, class P, const char* psLbl=psNil>
class HidVarCPT3DModel : public GenericHidVarCPTModel<Y,MapKey2D<X1,X2>,P,psLbl> {
  typedef GenericHidVarCPTModel<Y,MapKey2D<X1,X2>,P,psLbl> ParentType;
  typedef HidVarCPT3DModel<Y,X1,X2,P,psLbl> ThisType;
 public:
  P setIterProb ( typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2, int& vctr ) const {
    return GenericHidVarCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::setIterProb ( iyp, MapKey2D<X1,X2>(x1,x2), vctr );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2 ) {
    return GenericHidVarCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::setProb ( y, MapKey2D<X1,X2>(x1,x2) );
  }
  typename Y::template ArrayDistrib<P>& setDistrib ( const X1& x1, const X2& x2 ) {
    return GenericHidVarCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::setDistrib ( MapKey2D<X1,X2>(x1,x2) );
  }
  P getProb ( const typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2 ) const {
    return GenericHidVarCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::getProb ( iyp, MapKey2D<X1,X2>(x1,x2) );
  }
  const typename Y::template ArrayDistrib<P>& getDistrib ( const X1& x1, const X2& x2 ) const {
    return GenericHidVarCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::getDistrib ( MapKey2D<X1,X2>(x1,x2) );
  }
  bool contains ( const X1& x1, const X2& x2 ) const {
    return GenericHidVarCPTModel<Y,MapKey2D<X1,X2>,P,psLbl>::contains(MapKey2D<X1,X2>(x1,x2));
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};

////////////////////
template<class Y, class X1, class X2, class X3, class P, const char* psLbl=psNil>
class HidVarCPT4DModel : public GenericHidVarCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl> {
  typedef GenericHidVarCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl> ParentType;
  typedef HidVarCPT4DModel<Y,X1,X2,X3,P,psLbl> ThisType;
 public:
  P setIterProb ( typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2, const X3& x3, int& vctr ) const {
    return GenericHidVarCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::setIterProb ( iyp, MapKey3D<X1,X2,X3>(x1,x2,x3), vctr );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3 ) {
    return GenericHidVarCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::setProb ( y, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  typename Y::template ArrayDistrib<P>& setDistrib ( const X1& x1, const X2& x2, const X3& x3 ) {
    return GenericHidVarCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::setDistrib ( MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  P getProb ( const typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericHidVarCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::getProb ( iyp, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  const typename Y::template ArrayDistrib<P>& getDistrib ( const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericHidVarCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::getDistrib ( MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericHidVarCPTModel<Y,MapKey3D<X1,X2,X3>,P,psLbl>::contains(MapKey3D<X1,X2,X3>(x1,x2,x3));
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};

////////////////////
template<class Y, class X1, class X2, class X3, class X4, class P, const char* psLbl=psNil>
class HidVarCPT5DModel : public GenericHidVarCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl> {
  typedef GenericHidVarCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl> ParentType;
  typedef HidVarCPT5DModel<Y,X1,X2,X3,X4,P,psLbl> ThisType;
 public:
  P setIterProb ( typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, int& vctr ) const {
    return GenericHidVarCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::setIterProb ( iyp, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4), vctr );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) {
    return GenericHidVarCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::setProb ( y, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  typename Y::template ArrayDistrib<P>& setDistrib ( const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) {
    return GenericHidVarCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::setDistrib ( MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  P getProb ( const typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericHidVarCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::getProb ( iyp, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  const typename Y::template ArrayDistrib<P>& getDistrib ( const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericHidVarCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::getDistrib ( MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericHidVarCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P,psLbl>::contains(MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4));
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};

////////////////////
template<class Y, class X1, class X2, class X3, class X4, class X5, class P, const char* psLbl=psNil>
class HidVarCPT6DModel : public GenericHidVarCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl> {
  typedef GenericHidVarCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl> ParentType;
  typedef HidVarCPT6DModel<Y,X1,X2,X3,X4,X5,P,psLbl> ThisType;
 public:
  P setIterProb ( typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, int& vctr ) const {
    return GenericHidVarCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::setIterProb ( iyp, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5), vctr );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) {
    return GenericHidVarCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::setProb ( y, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  typename Y::template ArrayDistrib<P>& setDistrib ( const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) {
    return GenericHidVarCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::setDistrib ( MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  P getProb ( const typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericHidVarCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::getProb ( iyp, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  const typename Y::template ArrayDistrib<P>& getDistrib ( const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericHidVarCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::getDistrib ( MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericHidVarCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P,psLbl>::contains(MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5));
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};

////////////////////
template<class Y, class X1, class X2, class X3, class X4, class X5, class X6, class P, const char* psLbl=psNil>
class HidVarCPT7DModel : public GenericHidVarCPTModel<Y,MapKey6D<X1,X2,X3,X4,X5,X6>,P,psLbl> {
  typedef GenericHidVarCPTModel<Y,MapKey6D<X1,X2,X3,X4,X5,X6>,P,psLbl> ParentType;
  typedef HidVarCPT7DModel<Y,X1,X2,X3,X4,X5,X6,P,psLbl> ThisType;
 public:
  P setIterProb ( typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, const X6& x6, int& vctr ) const {
    return GenericHidVarCPTModel<Y,MapKey6D<X1,X2,X3,X4,X5,X6>,P,psLbl>::setIterProb ( iyp, MapKey6D<X1,X2,X3,X4,X5,X6>(x1,x2,x3,x4,x5,x6), vctr );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, const X6& x6  ) {
    return GenericHidVarCPTModel<Y,MapKey6D<X1,X2,X3,X4,X5,X6>,P,psLbl>::setProb ( y, MapKey6D<X1,X2,X3,X4,X5,X6>(x1,x2,x3,x4,x5,x6) );
  }
  typename Y::template ArrayDistrib<P>& setDistrib ( const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, const X6& x6 ) {
    return GenericHidVarCPTModel<Y,MapKey6D<X1,X2,X3,X4,X5,X6>,P,psLbl>::setDistrib ( MapKey6D<X1,X2,X3,X4,X5,X6>(x1,x2,x3,x4,x5,x6) );
  }
  P getProb ( const typename Y::template ArrayIterator<P>& iyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, const X6& x6 ) const {
    return GenericHidVarCPTModel<Y,MapKey6D<X1,X2,X3,X4,X5,X6>,P,psLbl>::getProb ( iyp, MapKey6D<X1,X2,X3,X4,X5,X6>(x1,x2,x3,x4,x5,x6) );
  }
  const typename Y::template ArrayDistrib<P>& getDistrib ( const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, const X6& x6 ) const {
    return GenericHidVarCPTModel<Y,MapKey6D<X1,X2,X3,X4,X5,X6>,P,psLbl>::getDistrib ( MapKey6D<X1,X2,X3,X4,X5,X6>(x1,x2,x3,x4,x5,x6) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, const X6& x6 ) const {
    return GenericHidVarCPTModel<Y,MapKey6D<X1,X2,X3,X4,X5,X6>,P,psLbl>::contains(MapKey6D<X1,X2,X3,X4,X5,X6>(x1,x2,x3,x4,x5,x6));
  }
  friend pair<IStream,ParentType*> operator>> ( IStream is, ThisType& m ) { return pair<IStream,ParentType*>(is,&m); }
};



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<class M1>
class SingleFactoredModel {
 private:
  M1 m1;
 public:
  const M1& getM1 ( )                             const { return m1; }
  M1&       setM1 ( )                             { return m1; }
  void subsume    ( SingleFactoredModel<M1>& fm ) { m1.subsume(fm.m1);             }
  void clear      ( )                             { m1.clear();                    }
  bool readFields ( Array<char*>& aps )           { return ( m1.readFields(aps) ); }
  friend pair<StringInput,SingleFactoredModel<M1>*> operator>> ( StringInput si, SingleFactoredModel<M1>& m ) {
    return pair<StringInput,SingleFactoredModel<M1>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,SingleFactoredModel<M1>*> si_m, const char* psD ) {
    return si_m.first>>si_m.second->m1>>psD; }
};

template<class M1,class M2>
class DoubleFactoredModel {
 private:
  M1 m1; M2 m2;
 public:
  const M1& getM1 ( )                                const { return m1; }
  const M2& getM2 ( )                                const { return m2; }
  M1&       setM1 ( )                                { return m1; }
  M2&       setM2 ( )                                { return m2; }
  void subsume    ( DoubleFactoredModel<M1,M2>& fm ) { m1.subsume(fm.m1); m2.subsume(fm.m2);                }
  void clear      ( )                                { m1.clear();        m2.clear();                       }
  bool readFields ( Array<char*>& aps )              { return ( m1.readFields(aps) || m2.readFields(aps) ); }
  friend pair<StringInput,DoubleFactoredModel<M1,M2>*> operator>> ( StringInput si, DoubleFactoredModel<M1,M2>& m ) {
    return pair<StringInput,DoubleFactoredModel<M1,M2>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,DoubleFactoredModel<M1,M2>*> si_m, const char* psD ) {
    StringInput si; return ( (si=si_m.first>>si_m.second->m1>>psD)!=NULL ||
                             (si=si_m.first>>si_m.second->m2>>psD)!=NULL ) ? si : NULL; }
};

template<class M1,class M2,class M3>
class TripleFactoredModel {
 private:
  M1 m1; M2 m2; M3 m3;
 public:
  const M1& getM1 ( )                                   const { return m1; }
  const M2& getM2 ( )                                   const { return m2; }
  const M3& getM3 ( )                                   const { return m3; }
  M1&       setM1 ( )                                   { return m1; }
  M2&       setM2 ( )                                   { return m2; }
  M3&       setM3 ( )                                   { return m3; }
  void subsume    ( TripleFactoredModel<M1,M2,M3>& fm ) { m1.subsume(fm.m1); m2.subsume(fm.m2); m3.subsume(fm.m3);                   }
  void clear      ( )                                   { m1.clear();        m2.clear();        m3.clear();                          }
  bool readFields ( Array<char*>& aps )                 { return ( m1.readFields(aps) || m2.readFields(aps) || m3.readFields(aps) ); }
  friend pair<StringInput,TripleFactoredModel<M1,M2,M3>*> operator>> ( StringInput si, TripleFactoredModel<M1,M2,M3>& m ) {
    return pair<StringInput,TripleFactoredModel<M1,M2,M3>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,TripleFactoredModel<M1,M2,M3>*> si_m, const char* psD ) {
    StringInput si; return ( (si=si_m.first>>si_m.second->m1>>psD)!=NULL ||
                             (si=si_m.first>>si_m.second->m2>>psD)!=NULL ||
                             (si=si_m.first>>si_m.second->m3>>psD)!=NULL ) ? si : NULL; }
};

template<class M1,class M2,class M3,class M4>
class QuadrupleFactoredModel {
 private:
  M1 m1; M2 m2; M3 m3;M4 m4;
 public:
  const M1& getM1 ( )                                   const { return m1; }
  const M2& getM2 ( )                                   const { return m2; }
  const M3& getM3 ( )                                   const { return m3; }
  const M4& getM4 ( )                                   const { return m4; }
  M1&       setM1 ( )                                   { return m1; }
  M2&       setM2 ( )                                   { return m2; }
  M3&       setM3 ( )                                   { return m3; }
  M4&       setM4 ( )                                   { return m4; }
  void subsume    ( QuadrupleFactoredModel<M1,M2,M3,M4>& fm ) { m1.subsume(fm.m1); m2.subsume(fm.m2); m3.subsume(fm.m3); m4.subsum(fm.m4); }
  void clear      ( )                                   { m1.clear();        m2.clear();        m3.clear();  m4.clear();                   }
  bool readFields ( Array<char*>& aps )                 { return ( m1.readFields(aps) || m2.readFields(aps) || m3.readFields(aps) || m4.readFields(aps) ); }
  friend pair<StringInput,QuadrupleFactoredModel<M1,M2,M3,M4>*> operator>> ( StringInput si, QuadrupleFactoredModel<M1,M2,M3,M4>& m ) {
    return pair<StringInput,QuadrupleFactoredModel<M1,M2,M3,M4>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,QuadrupleFactoredModel<M1,M2,M3,M4>*> si_m, const char* psD ) {
    StringInput si; return ( (si=si_m.first>>si_m.second->m1>>psD)!=NULL ||
                             (si=si_m.first>>si_m.second->m2>>psD)!=NULL ||
                             (si=si_m.first>>si_m.second->m3>>psD)!=NULL ||
                             (si=si_m.first>>si_m.second->m4>>psD)!=NULL ) ? si : NULL; }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/*
template<class IMRV1>
class ComplexSingleIteratedModeledRV {
 public:
  static const int NUM_ITERS = IMRV1::NUM_ITERS;
  IMRV1 iter_first;
  ComplexSingleIteratedModeledRV ( ) { }
  ComplexSingleIteratedModeledRV ( const ComplexSingleIteratedModeledRV& imrv ) : iter_first(imrv.iter_first) { }
  ComplexSingleIteratedModeledRV ( const IMRV1& imrv1 ) : iter_first(imrv1) { }
  void write ( FILE* pf ) const { iter_first.write(pf); }
  friend ostream& operator<< ( ostream& os, const ComplexSingleIteratedModeledRV<IMRV1>& rv ) { return os<<rv.iter_first; }
};

template<char* SD1,class IMRV1,char* SD2,class IMRV2,char* SD3>
class ComplexDoubleIteratedModeledRV {
 public:
  static const int NUM_ITERS = IMRV1::NUM_ITERS + IMRV2::NUM_ITERS;
//  typedef DelimitedJoint2DRV<SD1,typename IMRV1::VAR,SD2,typename IMRV2::VAR,SD3> VAR;
  IMRV1 iter_first;
  IMRV2 iter_second;
  ComplexDoubleIteratedModeledRV ( ) { }
  ComplexDoubleIteratedModeledRV ( const ComplexDoubleIteratedModeledRV& imrv ) : iter_first(imrv.iter_first), iter_second(imrv.iter_second) { }
  ComplexDoubleIteratedModeledRV ( const IMRV1& imrv1, const IMRV2& imrv2 ) : iter_first(imrv1), iter_second(imrv2) { }
//  operator VAR() { return VAR(iter_first,iter_second); }
  void write ( FILE* pf ) const { iter_first.write(pf); fprintf(pf,","); iter_second.write(pf); }
  friend ostream& operator<< ( ostream& os, const ComplexDoubleIteratedModeledRV<SD1,IMRV1,SD2,IMRV2,SD3>& rv ) { return os<<SD1<<rv.iter_first<<SD2<<rv.iter_second<<SD3; }
};

template<char* SD1,class IMRV1,char* SD2,class IMRV2,char* SD3,class IMRV3,char* SD4>
class ComplexTripleIteratedModeledRV {
 public:
  static const int NUM_ITERS = IMRV1::NUM_ITERS + IMRV2::NUM_ITERS + IMRV3::NUM_ITERS;
  IMRV1 iter_first;
  IMRV2 iter_second;
  IMRV3 iter_third;
  ComplexTripleIteratedModeledRV ( ) { }
  ComplexTripleIteratedModeledRV ( const ComplexTripleIteratedModeledRV& imrv ) : iter_first(imrv.iter_first), iter_second(imrv.iter_second), iter_third(imrv.iter_third) { }
  ComplexTripleIteratedModeledRV ( const IMRV1& imrv1, const IMRV2& imrv2, const IMRV3& imrv3 ) : iter_first(imrv1), iter_second(imrv2), iter_third(imrv3) { }
  void write ( FILE* pf ) const { iter_first.write(pf); fprintf(pf,","); iter_second.write(pf); fprintf(pf,","); iter_third.write(pf); }
  friend ostream& operator<< ( ostream& os, const ComplexTripleIteratedModeledRV<SD1,IMRV1,SD2,IMRV2,SD3,IMRV3,SD4>& rv )
    { return os<<SD1<<rv.iter_first<<SD2<<rv.iter_second<<SD3<<rv.iter_third<<SD4; }
};

template<char* SD1,class IMRV1,char* SD2,class IMRV2,char* SD3,class IMRV3,char* SD4,class IMRV4,char* SD5>
class ComplexQuadrupleIteratedModeledRV {
 public:
  static const int NUM_ITERS = IMRV1::NUM_ITERS + IMRV2::NUM_ITERS + IMRV3::NUM_ITERS + IMRV4::NUM_ITERS;
  IMRV1 iter_first;
  IMRV2 iter_second;
  IMRV3 iter_third;
  IMRV4 iter_fourth;
  ComplexQuadrupleIteratedModeledRV ( ) { }
  ComplexQuadrupleIteratedModeledRV ( const ComplexQuadrupleIteratedModeledRV& imrv ) : iter_first(imrv.iter_first), iter_second(imrv.iter_second),
                                                                                        iter_third(imrv.iter_third), iter_fourth(imrv.iter_fourth) { }
  ComplexQuadrupleIteratedModeledRV ( const IMRV1& imrv1, const IMRV2& imrv2, const IMRV3& imrv3, const IMRV4& imrv4 ) : iter_first(imrv1), iter_second(imrv2),
                                                                                                                         iter_third(imrv3), iter_fourth(imrv4) { }
  void write ( FILE* pf ) const { iter_first.write(pf); fprintf(pf,","); iter_second.write(pf); fprintf(pf,","); iter_third.write(pf); fprintf(pf,","); iter_fourth.write(pf); }
  friend ostream& operator<< ( ostream& os, const ComplexQuadrupleIteratedModeledRV<SD1,IMRV1,SD2,IMRV2,SD3,IMRV3,SD4,IMRV4,SD5>& rv )
    { return os<<SD1<<rv.iter_first<<SD2<<rv.iter_second<<SD3<<rv.iter_third<<SD4<<rv.iter_fourth<<SD5; }
};

template<class IMRV,char* SD,int I>
class ComplexArrayIteratedModeledRV {
 public:
  static const int NUM_ITERS = IMRV::NUM_ITERS * I;
  DelimitedStaticSafeArray<I,SD,IMRV> iter_array;
  void write ( FILE* pf ) const { for(int i=0;i<I;i++) iter_array.get(i).write(pf); }
  friend ostream& operator<< ( ostream& os, const ComplexArrayIteratedModeledRV<IMRV,SD,I>& rv ) { return os<<rv.iter_array; }
};
*/


#endif //_NL_CPT__
