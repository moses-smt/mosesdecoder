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

#ifndef _NL_RAND_VAR__
#define _NL_RAND_VAR__

#include <math.h>
#include <string>
#include "nl-string.h"
#include "nl-safeids.h"
#include "nl-stringindex.h"
#include "nl-prob.h"
#include "nl-hash.h"

////////////////////////////////////////////////////////////////////////////////

template <class A, class B, class C>
class trip {
 public:
  trip ( ) { }
  trip ( A& a, B& b, C& c ) : first(a), second(b), third(c) { }
  A first;
  B second;
  C third;
  friend ostream& operator<< ( ostream& os, const trip<A,B,C>& a ) { return os<<a.first<<","<<a.second<<","<<a.third; }
};

template <class A, class B, class C, class D>
class quad {
 public:
  quad ( ) { }
  quad ( A& a, B& b, C& c, D& d ) : first(a), second(b), third(c), fourth(d) { }
  A first;
  B second;
  C third;
  D fourth;
  friend ostream& operator<< ( ostream& os, const quad<A,B,C,D>& a ) { return os<<a.first<<","<<a.second<<","<<a.third<<","<<a.fourth; }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  DiscreteDomainRV template -- creates RV with a distinct set of values for domain T (unique class)
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template <class T>
class DiscreteDomain : public StringIndex {
 public:
  typedef T ValType;
  int MAX_SIZE ;
  DiscreteDomain ( )       : StringIndex()  { }
  DiscreteDomain ( int i ) : StringIndex()  { MAX_SIZE=i; }
  int addIndex ( const char* ps ) { int i=StringIndex::addIndex(ps); assert(i==T(i)); return i; }
};

////////////////////////////////////////////////////////////
template <class T, DiscreteDomain<T>& domain>
class DiscreteDomainRV : public Id<T> {
 private:

  static String strTemp;

 public:

  typedef DiscreteDomainRV<T,domain> BaseType;

  static const int NUM_VARS = 1;

  ////////////////////
  template<class P>
  class ArrayDistrib : public Array<pair<DiscreteDomainRV<T,domain>,P> > {
  };

  ////////////////////
  template<class P>
  class ArrayIterator : public pair<SafePtr<const ArrayDistrib<P> >,Id<int> > {
   public:
    static const int NUM_ITERS = NUM_VARS;
    operator DiscreteDomainRV<T,domain>() const { return ArrayIterator<P>::first.getRef().get(ArrayIterator<P>::second.toInt()).first; }
    //const DiscreteDomainRV<T,domain>& toRV() { return ArrayIterator<P>::first.getRef().get(ArrayIterator<P>::second.toInt()).first; }
    bool              end        ( ) const { return ( ArrayIterator<P>::second >= ArrayIterator<P>::first.getRef().getSize() ); }
    ArrayIterator<P>& operator++ ( )       { ++ArrayIterator<P>::second; return *this; }
  };

  // Static extraction methods...
  static const DiscreteDomain<T>& getDomain ( ) { return domain; }

  // Constructor / destructor methods...
  DiscreteDomainRV ( )                 { Id<T>::set(0); }
  DiscreteDomainRV ( int i )           { Id<T>::set(i); }
  DiscreteDomainRV ( const char* ps )  { assert(ps!=NULL); Id<T>::set(domain.addIndex(ps)); }

  // Specification methods...
  template<class P>
  DiscreteDomainRV<T,domain>& setVal ( const ArrayIterator<P>& it ) { *this=it; return *this; }
  bool setFirst ( )  { Id<T>::set(0); return isValid(); }
  bool setNext  ( )  { Id<T>::setNext(); if (!isValid()){Id<T>::set(0); return false;} return true; }

  // Extraction methods...
  bool   isValid   ( )                   const { return *this<domain.getSize(); } //return (this->Id<T>::operator<(domain.getSize())); }
  int    getIndex  ( )                   const { return Id<T>::toInt(); }  // DO NOT DELETE THIS METHOD!!!!!!!!!!
  string getString ( )                   const { return domain.getString(Id<T>::toInt()); }

  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const DiscreteDomainRV<T,domain>& rv ) { return  os<<rv.getString(); }
  friend String&  operator<< ( String& str, const DiscreteDomainRV<T,domain>& rv ) { return str<<rv.getString(); }
  friend pair<StringInput,DiscreteDomainRV<T,domain>*> operator>> ( const StringInput ps, DiscreteDomainRV<T,domain>& rv ) { return pair<StringInput,DiscreteDomainRV<T,domain>*>(ps,&rv); }
  friend StringInput operator>> ( pair<StringInput,DiscreteDomainRV<T,domain>*> delimbuff, const char* psDlm ) {
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    ////assert(*delimbuff.second<domain.getSize());
    int j=0;
    StringInput psIn = delimbuff.first;
    if(psDlm[0]=='\0') { *delimbuff.second=psIn.c_str(); return psIn+strlen(psIn.c_str()); }
    for(int i=0;psIn[i]!='\0';i++) {
      if(psIn[i]==psDlm[j]) j++;
      else j=0;
      strTemp[i]=psIn[i];
      if(j==int(strlen(psDlm))) { strTemp[i+1-j]='\0'; /*delimbuff.second->set(domain.addIndex(psIn.c_str()));*/ *delimbuff.second=strTemp.c_array(); return psIn+i+1;}
    }
    return NULL; //psIn;
  }
};
template <class T, DiscreteDomain<T>& domain>
String DiscreteDomainRV<T,domain>::strTemp ( 100 );


/* DON'T COMMENT BACK IN!!! THIS HAS BEEN MOVED TO nl-refrv.h!!!!!!
////////////////////////////////////////////////////////////
template <class T>
class RefRV : public Id<const T*> {
 public:

  typedef RefRV<T> BaseType;

  static const int NUM_VARS = 1;
  static const T   DUMMY;

  ////////////////////
  template<class P>
  class ArrayDistrib : public Array<pair<RefRV<T>,P> > {
  };

  ////////////////////
  template<class P>
  class ArrayIterator : public pair<SafePtr<const ArrayDistrib<P> >,Id<int> > {
   public:
    static const int NUM_ITERS = NUM_VARS;
    operator RefRV<T>() const { return ArrayIterator<P>::first.getRef().get(ArrayIterator<P>::second.toInt()).first; }
    //const DiscreteDomainRV<T,domain>& toRV() { return ArrayIterator<P>::first.getRef().get(ArrayIterator<P>::second.toInt()).first; }
  };

  // Constructor / destructor methods...
  RefRV ( )                 { Id<const T*>::set(NULL); }
  RefRV ( const T& t )      { Id<const T*>::set(&t);   }

  // Specification methods...
  template<class P>
  RefRV<T>& setVal ( const ArrayIterator<P>& it ) { *this=it; return *this; }

  // Extraction methods...
  const T& getRef ( ) const { return (Id<const T*>::toInt()==NULL) ? DUMMY : *(static_cast<const T*>(Id<const T*>::toInt())); }

  // Input / output methods..
  friend ostream& operator<< ( ostream& os, const RefRV<T>& rv ) { return os <<&rv.getRef(); }  //{ return  os<<rv.getRef(); }
  friend String&  operator<< ( String& str, const RefRV<T>& rv ) { return str<<"addr"<<(long int)(void*)&rv.getRef(); }  //{ return str<<rv.getRef(); }
  friend pair<StringInput,RefRV<T>*> operator>> ( const StringInput ps, RefRV<T>& rv ) { return pair<StringInput,RefRV<T>*>(ps,&rv); }
  friend StringInput operator>> ( pair<StringInput,RefRV<T>*> delimbuff, const char* psDlm ) {
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    return NULL; //psIn;
  }
};
template <class T> const T RefRV<T>::DUMMY;
*/


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  Joint2DRV
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
template<class V1,class V2>
class Joint2DRV {

 public:

  V1 first;
  V2 second;

  // Constructor / destructor methods...
  Joint2DRV ( )                            { }
  Joint2DRV ( const V1& v1, const V2& v2 ) { first=v1; second=v2; }

  // Extraction methods...
  size_t getHashKey ( ) const { size_t k=rotLeft(first.getHashKey(),3); k^=second.getHashKey();
                                /*fprintf(stderr,"  (%d) %d ^& %d = %d\n",sizeof(*this),x1.getHashKey(),x2.getHashKey(),k);*/ return k; }
  bool      operator< ( const Joint2DRV<V1,V2>& j )  const { return ( (first<j.first) ||
                                                                      (first==j.first && second<j.second) ); }
  bool      operator== ( const Joint2DRV<V1,V2>& j ) const { return ( first==j.first && second==j.second ); }
  bool      operator!= ( const Joint2DRV<V1,V2>& j ) const { return ( !(first==j.first && second==j.second) ); }
};


////////////////////////////////////////////////////////////
template<char* SD1,class V1,char* SD2,class V2,char* SD3>
class DelimitedJoint2DRV : public Joint2DRV<V1,V2> {

 public:

  static const int NUM_VARS = V1::NUM_VARS + V2::NUM_VARS;

  ////////////////////
  template<class P>
  class ArrayIterator : public pair<typename V1::template ArrayIterator<P>, typename V2::template ArrayIterator<P> > {
   public:
    // static const int NUM_ITERS = (typename V1::template ArrayIterator<P>)::NUM_ITERS + (typename V2::template ArrayIterator<P>)::NUM_ITERS;
    static const int NUM_ITERS = NUM_VARS;
//    DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>& set ( DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>& rv ) const { first.set(rv.first=first); rv.second=second; return rv; }
    friend ostream& operator<< ( ostream& os, const ArrayIterator<P>& rv ) {
      return os<<SD1<<rv.first<<SD2<<rv.second<<SD3; }
  };

  // Constructor / destructor methods...
  DelimitedJoint2DRV ( )                             : Joint2DRV<V1,V2>()      { }
  DelimitedJoint2DRV ( const V1& v1, const V2& v2 )  : Joint2DRV<V1,V2>(v1,v2) { }
  DelimitedJoint2DRV ( char* ps )                    : Joint2DRV<V1,V2>()      { ps>>*this>>"\0"; }
  DelimitedJoint2DRV ( const char* ps )              : Joint2DRV<V1,V2>()      { strdup(ps)>>*this>>"\0"; }  //DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>(strdup(ps)) { }

  // Specification methods...
  template<class P>
  DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>& setVal ( const ArrayIterator<P>& it ) {
    Joint2DRV<V1,V2>::first.setVal(it.first); Joint2DRV<V1,V2>::second.setVal(it.second); return *this; }

  // Extraction methods...
  bool operator==(const DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>& vv) const { return Joint2DRV<V1,V2>::operator==(vv); }
  bool operator< (const DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>& vv) const { return Joint2DRV<V1,V2>::operator<(vv); }

  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>& rv ) { return  os<<SD1<<rv.first<<SD2<<rv.second<<SD3; }
  friend String&  operator<< ( String& str, const DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>& rv ) { return str<<SD1<<rv.first<<SD2<<rv.second<<SD3; }
  friend IStream operator>> ( pair<IStream,DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>*> is_x, const char* psDlm ) {
    IStream&                               is =  is_x.first;
    DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>& x  = *is_x.second;
    // Propagate fail...
    if ( IStream()==is ) return is;
    // Use last delimiter only if not empty (otherwise it will immediately trivially match)...
    return ( (SD3[0]=='\0') ? is>>SD1>>x.first>>SD2>>x.second>>psDlm
                            : is>>SD1>>x.first>>SD2>>x.second>>SD3>>psDlm );
  }

  // OBSOLETE!
  friend pair<StringInput,DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>*> operator>> ( StringInput ps, DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>& rv ) { return pair<StringInput,DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>*>(ps,&rv); }
  friend StringInput    operator>> ( pair<StringInput,DelimitedJoint2DRV<SD1,V1,SD2,V2,SD3>*> delimbuff, const char* psDlm ) {
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    return ( (SD3[0]=='\0') ? delimbuff.first>>SD1>>delimbuff.second->first>>SD2>>delimbuff.second->second>>psDlm
                            : delimbuff.first>>SD1>>delimbuff.second->first>>SD2>>delimbuff.second->second>>SD3>>psDlm );
  }
};



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  Joint3DRV
//
////////////////////////////////////////////////////////////////////////////////

template<class V1,class V2,class V3>
class Joint3DRV {

 public:

  V1 first;
  V2 second;
  V3 third;

  // Constructor / destructor methods...
  Joint3DRV ( )                                          { }
  Joint3DRV ( const V1& v1, const V2& v2, const V3& v3 ) { first=v1; second=v2; third=v3; }

  /*
  // Specification methods...
  bool                 operator< ( const Joint3DRV<V1,V2,V3>& j ) const {
    return ( (x1<j.x1) ||
             (x1==j.x1 && x2<j.x2) ||
             (x1==j.x1 && x2==j.x2 && x3<j.x3) ) ;
  }
  */

  // Extraction methods...
  size_t getHashKey ( ) const { size_t k=rotLeft(first.getHashKey(),3); k^=second.getHashKey(); k=rotLeft(k,3); k^=third.getHashKey();
                                /*fprintf(stderr,"  (%d) %d ^& %d = %d\n",sizeof(*this),x1.getHashKey(),x2.getHashKey(),k);*/ return k; }
//  bool      operator< ( const Joint2DRV<V1,V2>& j )  const { return ( (first<j.first) ||
//                                                                      (first==j.first && second<j.second) ); }
  bool      operator== ( const Joint3DRV<V1,V2,V3>& j ) const { return ( first==j.first && second==j.second && third==j.third ); }
  bool      operator!= ( const Joint3DRV<V1,V2,V3>& j ) const { return ( !(first==j.first && second==j.second && third==j.third) ); }
};

////////////////////////////////////////////////////////////
template<char* SD1,class V1,char* SD2,class V2,char* SD3,class V3,char* SD4>
class DelimitedJoint3DRV : public Joint3DRV<V1,V2,V3> {

 public:

  static const int NUM_VARS = V1::NUM_VARS + V2::NUM_VARS + V3::NUM_VARS;

  ////////////////////
  template<class P>
  class ArrayIterator : public trip<typename V1::template ArrayIterator<P>, typename V2::template ArrayIterator<P>, typename V3::template ArrayIterator<P> > {
   public:
    // static const int NUM_ITERS = (typename V1::template ArrayIterator<P>)::NUM_ITERS + (typename V2::template ArrayIterator<P>)::NUM_ITERS;
    static const int NUM_ITERS = NUM_VARS;
    friend ostream& operator<< ( ostream& os, const ArrayIterator<P>& rv ) {
      return os<<SD1<<rv.first<<SD2<<rv.second<<SD3<<rv.third<<SD4; }
  };

  // Constructor / destructor methods...
  DelimitedJoint3DRV ( )                                           : Joint3DRV<V1,V2,V3>()         { }
  DelimitedJoint3DRV ( const V1& v1, const V2& v2, const V3& v3 )  : Joint3DRV<V1,V2,V3>(v1,v2,v3) { }
  DelimitedJoint3DRV ( char* ps )                                  : Joint3DRV<V1,V2,V3>()         { ps>>*this>>"\0"; }
  DelimitedJoint3DRV ( const char* ps )                            : Joint3DRV<V1,V2,V3>()         { strdup(ps)>>*this>>"\0"; }

  // Specification methods...
  template<class P>
  DelimitedJoint3DRV<SD1,V1,SD2,V2,SD3,V3,SD4>& setVal ( const ArrayIterator<P>& it ) {
    Joint3DRV<V1,V2,V3>::first.setVal(it.first); Joint3DRV<V1,V2,V3>::second.setVal(it.second); Joint3DRV<V1,V2,V3>::third.setVal(it.third); return *this; }

  // Extraction methods...
  bool operator==(const DelimitedJoint3DRV<SD1,V1,SD2,V2,SD3,V3,SD4>& vvv) const { return Joint3DRV<V1,V2,V3>::operator==(vvv); }
  bool operator< (const DelimitedJoint3DRV<SD1,V1,SD2,V2,SD3,V3,SD4>& vvv) const { return Joint3DRV<V1,V2,V3>::operator< (vvv); }

  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const DelimitedJoint3DRV<SD1,V1,SD2,V2,SD3,V3,SD4>& rv ) { return  os<<SD1<<rv.first<<SD2<<rv.second<<SD3<<rv.third<<SD4; }
  friend String&  operator<< ( String& str, const DelimitedJoint3DRV<SD1,V1,SD2,V2,SD3,V3,SD4>& rv ) { return str<<SD1<<rv.first<<SD2<<rv.second<<SD3<<rv.third<<SD4; }
  friend pair<StringInput,DelimitedJoint3DRV<SD1,V1,SD2,V2,SD3,V3,SD4>*> operator>> ( StringInput ps, DelimitedJoint3DRV<SD1,V1,SD2,V2,SD3,V3,SD4>& rv ) {
    return pair<StringInput,DelimitedJoint3DRV<SD1,V1,SD2,V2,SD3,V3,SD4>*>(ps,&rv); }
  friend StringInput    operator>> ( pair<StringInput,DelimitedJoint3DRV<SD1,V1,SD2,V2,SD3,V3,SD4>*> delimbuff, const char* psDlm ) {
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    return ( (SD4[0]=='\0') ? delimbuff.first>>SD1>>delimbuff.second->first>>SD2>>delimbuff.second->second>>SD3>>delimbuff.second->third>>psDlm
                            : delimbuff.first>>SD1>>delimbuff.second->first>>SD2>>delimbuff.second->second>>SD3>>delimbuff.second->third>>SD4>>psDlm );
  }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  Joint4DRV
//
////////////////////////////////////////////////////////////////////////////////

template<class V1,class V2,class V3, class V4>
class Joint4DRV {

 public:

  V1 first;
  V2 second;
  V3 third;
  V4 fourth;

  // Constructor / destructor methods...
  Joint4DRV ( )                                          { }
  Joint4DRV ( const V1& v1, const V2& v2, const V3& v3, const V4& v4 ) { first=v1; second=v2; third=v3; fourth=v4;}
  // Extraction methods...
  size_t getHashKey ( ) const { size_t k=rotLeft(first.getHashKey(),3); k^=second.getHashKey(); k=rotLeft(k,3); k^=third.getHashKey();k^=fourth.getHashKey();
                                /*fprintf(stderr,"  (%d) %d ^& %d = %d\n",sizeof(*this),x1.getHashKey(),x2.getHashKey(),k);*/ return k; }
//  bool      operator< ( const Joint2DRV<V1,V2>& j )  const { return ( (first<j.first) ||
//                                                                      (first==j.first && second<j.second) ); }
  bool      operator== ( const Joint4DRV<V1,V2,V3,V4>& j ) const { return ( first==j.first && second==j.second && third==j.third && fourth==j.fourth ); }
  bool      operator!= ( const Joint4DRV<V1,V2,V3,V4>& j ) const { return ( !(first==j.first && second==j.second && third==j.third && fourth==j.fourth) ); }
};

////////////////////////////////////////////////////////////
template<char* SD1,class V1,char* SD2,class V2,char* SD3,class V3,char* SD4,class V4, char* SD5>
class DelimitedJoint4DRV : public Joint4DRV<V1,V2,V3,V4> {

 public:

  static const int NUM_VARS = V1::NUM_VARS + V2::NUM_VARS + V3::NUM_VARS+ V4::NUM_VARS;

  ////////////////////
  template<class P>
  class ArrayIterator : public quad<typename V1::template ArrayIterator<P>, typename V2::template ArrayIterator<P>, typename V3::template ArrayIterator<P> , typename V4::template ArrayIterator<P> > {
   public:
    // static const int NUM_ITERS = (typename V1::template ArrayIterator<P>)::NUM_ITERS + (typename V2::template ArrayIterator<P>)::NUM_ITERS;
    static const int NUM_ITERS = NUM_VARS;
    friend ostream& operator<< ( ostream& os, const ArrayIterator<P>& rv ) {
      return os<<SD1<<rv.first<<SD2<<rv.second<<SD3<<rv.third<<SD4<<rv.fourth<<SD5; }
  };

  // Constructor / destructor methods...
  DelimitedJoint4DRV ( )                                           : Joint4DRV<V1,V2,V3,V4>()         { }
  DelimitedJoint4DRV ( const V1& v1, const V2& v2, const V3& v3, const V4& v4 )  : Joint4DRV<V1,V2,V3,V4>(v1,v2,v3,v4) { }
  DelimitedJoint4DRV ( char* ps )                                  : Joint4DRV<V1,V2,V3,V4>()         { ps>>*this>>"\0"; }
  DelimitedJoint4DRV ( const char* ps )                            : Joint4DRV<V1,V2,V3,V4>()         { strdup(ps)>>*this>>"\0"; }

  // Specification methods...
  template<class P>
  DelimitedJoint4DRV<SD1,V1,SD2,V2,SD3,V3,SD4,V4,SD5>& setVal ( const ArrayIterator<P>& it ) {
    Joint4DRV<V1,V2,V3,V4>::first.setVal(it.first);
    Joint4DRV<V1,V2,V3,V4>::second.setVal(it.second);
    Joint4DRV<V1,V2,V3,V4>::third.setVal(it.third);
    Joint4DRV<V1,V2,V3,V4>::fourth.setVal(it.fourth);
    return *this;
    }

  // Extraction methods...
  bool operator==(const DelimitedJoint4DRV<SD1,V1,SD2,V2,SD3,V3,SD4,V4,SD5>& vvvv) const { return Joint4DRV<V1,V2,V3,V4>::operator==(vvvv); }
  bool operator< (const DelimitedJoint4DRV<SD1,V1,SD2,V2,SD3,V3,SD4,V4,SD5>& vvvv) const { return Joint4DRV<V1,V2,V3,V4>::operator< (vvvv); }

  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const DelimitedJoint4DRV<SD1,V1,SD2,V2,SD3,V3,SD4,V4,SD5>& rv ) { return  os<<SD1<<rv.first<<SD2<<rv.second<<SD3<<rv.third<<SD4<<rv.fourth<<SD5; }
  friend String&  operator<< ( String& str, const DelimitedJoint4DRV<SD1,V1,SD2,V2,SD3,V3,SD4,V4,SD5>& rv ) { return str<<SD1<<rv.first<<SD2<<rv.second<<SD3<<rv.third<<SD4<<rv.fourth<<SD5; }
  friend pair<StringInput,DelimitedJoint4DRV<SD1,V1,SD2,V2,SD3,V3,SD4,V4,SD5>*> operator>> ( StringInput ps, DelimitedJoint4DRV<SD1,V1,SD2,V2,SD3,V3,SD4,V4,SD5>& rv ) {
    return pair<StringInput,DelimitedJoint4DRV<SD1,V1,SD2,V2,SD3,V3,SD4,V4,SD5>*>(ps,&rv); }
  friend StringInput    operator>> ( pair<StringInput,DelimitedJoint4DRV<SD1,V1,SD2,V2,SD3,V3,SD4,V4,SD5>*> delimbuff, const char* psDlm ) {
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    return ( (SD5[0]=='\0') ? delimbuff.first>>SD1>>delimbuff.second->first>>SD2>>delimbuff.second->second>>SD3>>delimbuff.second->third>>SD4>>delimbuff.second->fourth>>psDlm
                            : delimbuff.first>>SD1>>delimbuff.second->first>>SD2>>delimbuff.second->second>>SD3>>delimbuff.second->third>>SD4>>delimbuff.second->fourth>>SD5>>psDlm );
  }
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  JointArrayRV<T,I>
//
////////////////////////////////////////////////////////////////////////////////

template <int I, class T>
class JointArrayRV {
 private:
  // Data members...
  T at[I];
 public:
  typedef T ElementType;

  /*
  // Constructor / destructor methods...
  JointArrayRV ( )              { }
  JointArrayRV ( const T& t )   { for(int i=0;i<I;i++) at[i]=t; }
  */

  // Static extraction methods...
  static const int SIZE = I;
  static const int getSize ( )       { return I; }

  // Specification methods...
  T&         set       (int i)       { assert(0<=i); assert(i<I); return at[i]; }

  // Extraction methods...
  const T&   get       (int i) const { assert(NULL!=this); assert(0<=i); assert(i<I); return at[i]; }
  bool       operator< ( const JointArrayRV<I,T>& a ) const {
    int i;
    for ( i=0; at[i]==a.at[i] && i<I; i++ ) ;
    return ( i<I && at[i]<a.at[i] ) ;
  }
  bool       operator== ( const JointArrayRV<I,T>& a ) const {
    int i;
    for ( i=0; at[i]==a.at[i] && i<I; i++ ) ;
    return ( i==I ) ;
  }
  size_t getHashKey   ( ) const { size_t k=0; for(int i=0;i<I;i++){k=rotLeft(k,3); k^=get(i).getHashKey(); } return k; }
};

////////////////////////////////////////////////////////////////////////////////

template <int I, char* SD, class T>
class DelimitedJointArrayRV : public JointArrayRV<I,T> {
 public:

  static const int NUM_VARS = T::NUM_VARS * I;

  ////////////////////
  template<class P>
  class ArrayIterator : public StaticSafeArray<I,typename T::template ArrayIterator<P> > {
   public:
    static const int NUM_ITERS = NUM_VARS;
    // static const int NUM_ITERS = (typename T::template ArrayIterator<P>)::NUM_ITERS * I;
    friend ostream& operator<< ( ostream& os, const ArrayIterator<P>& rv ) { for(int i=0;i<I;i++) os<<((i==0)?"":SD)<<rv.get(i); return os; }
  };

  // Specification methods...
  template<class P>
  DelimitedJointArrayRV<I,SD,T>& setVal ( const ArrayIterator<P>& it ) {
    for(int i=0;i<I;i++) JointArrayRV<I,T>::set(i).setVal(it.get(i)); return *this; }

  // Extraction methods...
  bool operator==(const DelimitedJointArrayRV<I,SD,T>& a) const { return JointArrayRV<I,T>::operator==(a); }
  bool operator< (const DelimitedJointArrayRV<I,SD,T>& a) const { return JointArrayRV<I,T>::operator<(a); }

  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const DelimitedJointArrayRV<I,SD,T>& a ) { for(int i=0;i<I;i++) os<<((i==0)?"":SD)<<a.get(i); return os;  }
  friend String&  operator<< ( String& str, const DelimitedJointArrayRV<I,SD,T>& a ) { for(int i=0;i<I;i++)str<<((i==0)?"":SD)<<a.get(i); return str; }
  friend IStream operator>> ( pair<IStream,DelimitedJointArrayRV<I,SD,T>*> is_x, const char* psDlm ) {
    IStream&                       is =  is_x.first;
    DelimitedJointArrayRV<I,SD,T>& x  = *is_x.second;
    if (IStream()==is) return IStream();
    for(int i=0;i<I;i++)
      is = pair<IStream,T*>(is,&x.set(i))>>((i<I-1)?SD:psDlm);
    return is;
  }

  // OBSOLETE!
  friend pair<StringInput,DelimitedJointArrayRV<I,SD,T>*> operator>> ( StringInput ps, DelimitedJointArrayRV<I,SD,T>& a ) { return pair<StringInput,DelimitedJointArrayRV<I,SD,T>*>(ps,&a); }
  friend StringInput operator>> ( pair<StringInput,DelimitedJointArrayRV<I,SD,T>*> delimbuff, const char* psDlm ) {
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    StringInput psIn = delimbuff.first;
    for(int i=0;i<I;i++)
      psIn = pair<StringInput,T*>(psIn,&delimbuff.second->set(i))>>((i<I-1)?SD:psDlm);
    return psIn;
  }
};


///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  History<T,N>
//
////////////////////////////////////////////////////////////////////////////////

template <int N,class T>
class History {
 private:
  // Data members...
  StaticSafeArray<N,T> at;
 public:
  // Constructor / destructor methods...
  History ( )                 { }
  History ( char* ps )        { ps>>*this>>"\0"; }
  /*
  History ( char* ps )        { read(ps); }
  */
//  History ( const string& s ) { read(s.c_str()); }
  // Specification methods...
  void advanceHistory(const T& t) { for(int i=N-1;i>0;i--)at.set(i)=at.get(i-1); at.set(0)=t; }
  T&   advanceHistory()           { for(int i=N-1;i>0;i--)at.set(i)=at.get(i-1); return at.set(0); }
  T&   setBack(int i)             { return at.set(i); }
  // Extraction methods...
  const T& getBack(int i) const { assert(i>=0); assert(i<N); return at.get(i); }
  // Input / output methods...
  /*
  void read ( char* ps, const ReaderContext& rc=ReaderContext() ) { char* psT; for(int i=0;i<N;i++){char* z=strtok_r((0==i)?ps:NULL,";",&psT); assert(z); at.set(i).read(z);} }
  //at.set(i).read(strtok_r((0==i)?ps:NULL,";",&psT)); }
  */

  friend ostream& operator<< ( ostream& os, const History<N,T>& a ) { for(int i=0;i<N;i++)os<<((i==0)?"":";")<<a.getBack(i); return os; }
  friend pair<StringInput,History<N,T>*> operator>> ( StringInput ps, History<N,T>& a ) { return pair<StringInput,History<N,T>*>(ps,&a); }
  friend StringInput    operator>> ( pair<StringInput,History<N,T>*> delimbuff, const char* psDlm ) {
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    StringInput psIn = delimbuff.first;
    for(int i=0;i<N;i++)
      psIn = pair<StringInput,T*>(psIn,&delimbuff.second->setBack(i))>>((i<N-1)?";":psDlm);
    return psIn;
  }

  /*
  void write ( FILE* pf ) const { for(int i=0;i<N;i++) {fprintf(pf,(0==i)?"":";"); at.get(i).write(pf);} }
  */
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


#endif //_NL_RAND_VAR__
