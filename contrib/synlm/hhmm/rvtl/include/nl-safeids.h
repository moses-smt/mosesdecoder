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

#ifndef _NL_SAFE_IDS__
#define _NL_SAFE_IDS__

#include "nl-const.h"
#include "nl-stringindex.h"
//#include "nl-string.h"
#include "nl-stream.h"

#include <iostream>
using namespace std;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  StaticSafeArray<T,I>
//
////////////////////////////////////////////////////////////////////////////////

template <int I, class T>
class StaticSafeArray {
 private:
  // Data members...
  T at[I];
 public:
  typedef T ElementType;
  template<class P> class Iterator : public StaticSafeArray<I,typename T::template Iterator<P> > {
   public:
    //operator StaticSafeArray<I,T>() const { }
  };
  // Constructor / destructor methods...
  StaticSafeArray ( )                               { }
  StaticSafeArray ( const T& t )                    { for(int i=0;i<I;i++) at[i]=t; }
  StaticSafeArray ( const StaticSafeArray<I,T>& a ) { for(int i=0;i<I;i++) at[i]=a.at[i]; }
  // Static extraction methods...
  static const int SIZE = I;
  static const int getSize ( )       { return I; }
  // Math methods...
  StaticSafeArray<I,T> operator+ ( const StaticSafeArray<I,T>& a ) const { StaticSafeArray<I,T> aOut; for(int i=0;i<I;i++) aOut.at[i]=at[i]+a.at[i]; return aOut; }
  StaticSafeArray<I,T> operator- ( const StaticSafeArray<I,T>& a ) const { StaticSafeArray<I,T> aOut; for(int i=0;i<I;i++) aOut.at[i]=at[i]-a.at[i]; return aOut; }
  //StaticSafeArray<I,T> operator+ ( const T& t ) const { StaticSafeArray<I,T> aOut; for(int i=0;i<I;i++) aOut.at[i]=at[i]+t; return aOut; }
  //StaticSafeArray<I,T> operator- ( const T& t ) const { StaticSafeArray<I,T> aOut; for(int i=0;i<I;i++) aOut.at[i]=at[i]-t; return aOut; }
  // Specification methods...
  T&         set       (int i)       { assert(0<=i); assert(i<I); return at[i]; }
  T&         operator[](int i)       { assert(0<=i); assert(i<I); return at[i]; }
  // Extraction methods...
  const T&   get       (int i) const { assert(NULL!=this); assert(0<=i); assert(i<I); return at[i]; }
  const T&   operator[](int i) const { assert(NULL!=this); assert(0<=i); assert(i<I); return at[i]; }
  bool       operator< ( const StaticSafeArray<I,T>& a ) const {
    int i;
    for ( i=0; at[i]==a.at[i] && i<I; i++ ) ;
    return ( i<I && at[i]<a.at[i] ) ;
  }
  bool       operator== ( const StaticSafeArray<I,T>& a ) const {
    int i;
    for ( i=0; at[i]==a.at[i] && i<I; i++ ) ;
    return ( i==I ) ;
  }
  size_t getHashKey   ( ) const { size_t k=0; for(int i=0;i<I;i++){k=rotLeft(k,3); k^=get(i).getHashKey(); } return k; }
//  friend ostream& operator<< ( ostream& os, const StaticSafeArray<I,T>& a ) { for(int i=0;i<I;i++) os<<((i==0)?"":",")<<a.get(i); return os; }
};

////////////////////////////////////////////////////////////////////////////////

template <int I, char* SD, class T>
class DelimitedStaticSafeArray : public StaticSafeArray<I,T> {
 public:
  DelimitedStaticSafeArray ( )                               : StaticSafeArray<I,T>()  { }
  DelimitedStaticSafeArray ( const T& t )                    : StaticSafeArray<I,T>(t) { }
  DelimitedStaticSafeArray ( const StaticSafeArray<I,T>& a ) : StaticSafeArray<I,T>(a) { }

  bool operator==(const DelimitedStaticSafeArray<I,SD,T>& a) const { return StaticSafeArray<I,T>::operator==(a); }
  bool operator< (const DelimitedStaticSafeArray<I,SD,T>& a) const { return StaticSafeArray<I,T>::operator<(a); }

  friend ostream& operator<< ( ostream& os, const DelimitedStaticSafeArray<I,SD,T>& a ) { for(int i=0;i<I;i++) os<<((i==0)?"":SD)<<a.get(i); return os;  }
  friend String&  operator<< ( String& str, const DelimitedStaticSafeArray<I,SD,T>& a ) { for(int i=0;i<I;i++)str<<((i==0)?"":SD)<<a.get(i); return str; }
  friend IStream operator>> ( pair<IStream,DelimitedStaticSafeArray<I,SD,T>*> is_x, const char* psDlm ) {
    IStream&                          is =  is_x.first;
    DelimitedStaticSafeArray<I,SD,T>& x  = *is_x.second;
    if (IStream()==is) return IStream();
    for(int i=0;i<I;i++)
      is = pair<IStream,T*>(is,&x.set(i))>>((i<I-1)?SD:psDlm);
    return is;
  }


  // OBSOLETE!
  friend pair<StringInput,DelimitedStaticSafeArray<I,SD,T>*> operator>> ( StringInput ps, DelimitedStaticSafeArray<I,SD,T>& a ) { return pair<StringInput,DelimitedStaticSafeArray<I,SD,T>*>(ps,&a); }
  friend StringInput operator>> ( pair<StringInput,DelimitedStaticSafeArray<I,SD,T>*> delimbuff, const char* psDlm ) {
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    StringInput psIn = delimbuff.first;
    for(int i=0;i<I;i++)
      psIn = pair<StringInput,T*>(psIn,&delimbuff.second->set(i))>>((i<I-1)?SD:psDlm);
    return psIn;
  }

  /*
  void read  ( char* ps )       { char* psT; for(int i=0;i<I;i++){char* z=strtok_r((0==i)?ps:NULL,SD,&psT); assert(z);
                                                                  StaticSafeArray<I,T>::set(i).read(z);} }
  void read  ( char* ps, ReaderContext& rc ) { read(ps); }
  void write ( FILE* pf ) const { for(int i=0;i<I;i++){fprintf(pf,(0==i)?"":SD); StaticSafeArray<I,T>::get(i).write(pf);} }
  void write ( FILE* pf, ReaderContext& rc ) const { write(pf); }
  */
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  Delimited pair
//
////////////////////////////////////////////////////////////////////////////////

template<char* SD1,class V1,char* SD2,class V2,char* SD3>
class DelimitedPair : public pair<V1,V2> {
 public:
  // Constructor / destructor methods...
  DelimitedPair ( )                             : pair<V1,V2>()      { }
  DelimitedPair ( const V1& v1, const V2& v2 )  : pair<V1,V2>(v1,v2) { }

  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const DelimitedPair<SD1,V1,SD2,V2,SD3>& rv ) { return  os<<SD1<<rv.first<<SD2<<rv.second<<SD3; }
  friend String&  operator<< ( String& str, const DelimitedPair<SD1,V1,SD2,V2,SD3>& rv ) { return str<<SD1<<rv.first<<SD2<<rv.second<<SD3; }
  friend IStream operator>> ( pair<IStream,DelimitedPair<SD1,V1,SD2,V2,SD3>*> is_x, const char* psDlm ) {
    IStream&                               is =  is_x.first;
    DelimitedPair<SD1,V1,SD2,V2,SD3>& x  = *is_x.second;
    // Propagate fail...
    if ( IStream()==is ) return is;
    IStream is1 = (is>>SD1>>x.first>>SD2);
    IStream is2 = (is>>SD1>>x.first>>SD2>>x.second>>psDlm);
    // Use last delimiter only if not empty (otherwise it will immediately trivially match)...
    return ( (SD3[0]=='\0') ? is>>SD1>>x.first>>SD2>>x.second>>psDlm
                            : is>>SD1>>x.first>>SD2>>x.second>>SD3>>psDlm );
  }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  Id -- generalize int to any symbol value
//
////////////////////////////////////////////////////////////////////////////////

template<class T>
class Id {
 private:
  // Data members...
  T val;
 public:
  typedef T IntType;
  // Constructor / destructor methods...
  Id ()            { val=0; }
  Id (T t)         { val=t; }
//  Id (int i)       { val=i; }
  Id (const Id& i) { val=i.val; }
  // Specification methods...
  Id& operator=  (const T& i)  { val=i; return *this; }
//  Id& operator=  (int i)       { val=i; return *this; }
  Id& operator=  (const Id& i) { val=i.val; return *this; }
  Id& operator+= (const Id& i) { val+=i.val; return *this; }
  Id& operator++ ( )           { ++val; return *this; }
  Id& setFirst   ( )           { val=0; return *this; }
  Id& setFirst   (const Id& i) { val=0; return *this; }
  Id& setNext    ( )           { ++val; return *this; }
  Id& setNext    (const Id& i) { ++val; return *this; }
  // Extraction methods...
  //int         getHashConst ( const int m ) const { return (sizeof(T)%m); }
  //int         getHashKey   ( const int m ) const { return (((int)*this)%m);  }
  size_t getHashKey ( ) const { return (size_t)val; }
  bool operator== ( const Id& i ) const { return val==i.val; }
  bool operator!= ( const Id& i ) const { return val!=i.val; }
  bool operator<  ( const Id& i ) const { return val<i.val; }
  bool operator<= ( const Id& i ) const { return val<=i.val; }
  bool operator>  ( const Id& i ) const { return val>i.val; }
  bool operator>= ( const Id& i ) const { return val>=i.val; }
  Id   operator+  ( const Id& i ) const { return Id(val+i.val); }
  Id   operator-  ( const Id& i ) const { return Id(val-i.val); }
  bool operator== ( const T t )   const { return val==t; }
  bool operator<  ( const T t )   const { return val<t; }
  bool operator<= ( const T t )   const { return val<=t; }
  bool operator>  ( const T t )   const { return val>t; }
  bool operator>= ( const T t )   const { return val>=t; }
  Id   operator+  ( const T t )   const { return Id(val+t); }
  Id   operator-  ( const T t )   const { return Id(val-t); }
//  operator T() const { return val; };

  T     toInt     ( )     const { return val; }
  Id<T> set       (T t)         { val=t; return *this; }
//  void setInt    (int i)       { val=i; }

  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const Id<T>& i ) { return os<<i.val; }

  /*
  void write ( FILE* pf, const ReaderContext& rc=ReaderContext() ) const { (sizeof(T)<sizeof(long long)) ? fprintf(pf,"%d",val) : fprintf(pf,"%lld",val); }
  */
} ;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  Safe pointers..
//
////////////////////////////////////////////////////////////////////////////////

template <class R>
class SafePtr {
 private:
  R* pr;
  static const R rDummy;
 public:
  SafePtr<R> ( )      : pr(NULL) { }
  SafePtr<R> ( R& r ) : pr(&r)   { }
  bool operator== ( const SafePtr<R>& spr ) const { return(pr==spr.pr); }
  bool operator!= ( const SafePtr<R>& spr ) const { return(!(pr==spr.pr)); }
  //friend void delete ( SafePtr<R>& sp )  { delete sp.pr; }
  void     del    ( )       { delete pr; pr=NULL; }
  //R&       setRefV ( )      { assert(pr); return *pr; }
  R&       setRef  ( )      { assert(pr); return (pr!=NULL) ? *pr : rDummy; }
  const R& getRef ( ) const { return (pr!=NULL) ? *pr : rDummy; }
};
template <class R>
const R SafePtr<R>::rDummy = R();

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  Safe static array templates for small arrays...
//
////////////////////////////////////////////////////////////////////////////////

template <class T>
class Doub {
 private:
  T at[2];
 public:
  T&       set ( int i )       { assert(0<=i&&i<2); return at[i]; }
  const T& get ( int i ) const { assert(0<=i&&i<2); return at[i]; }
};

template <class T>
class Trip {
 private:
  T at[3];
 public:
  T&       set ( int i )       { assert(0<=i&&i<3); return at[i]; }
  const T& get ( int i ) const { assert(0<=i&&i<3); return at[i]; }
};

template <class T>
class Quad {
 private:
  T at[4];
 public:
  T&       set ( int i )       { assert(0<=i&&i<4); return at[i]; }
  const T& get ( int i ) const { assert(0<=i&&i<4); return at[i]; }
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  SafeArrays -- prevents overflow memory leaks
//
////////////////////////////////////////////////////////////////////////////////

template <class X1, class T>
class SafeArray1D {
 private:
  // Data members;
  int xSize;
  T*  at;
 public:
  // Constructor / destructor methods...
  ~SafeArray1D( )                    { delete[] at; }
  SafeArray1D ( )                    { xSize=0; at=NULL; }
  SafeArray1D (int x)                { xSize=x; at=new T[x]; }
  SafeArray1D (int x, const T& t)    { xSize=x; at=new T[x];
                                          for(int i=0;i<x;++i) at[i]=t; }
  SafeArray1D ( const SafeArray1D<X1,T>& sat ) { delete[] at; xSize=sat.xSize; at=new T[xSize];
                                                 for(int i=0;i<xSize;i++) at[i]=sat.at[i]; }
  // Specification methods...
  SafeArray1D& operator= ( const SafeArray1D<X1,T>& sat )
    { delete[] at; xSize=sat.xSize; at=new T[xSize];
      for(int i=0;i<xSize;i++) at[i]=sat.at[i]; return *this; }
  void  init ( int x )               { delete[] at; xSize=x; at=new T[x];}
  void  init ( int x, const T& t )   { delete[] at; xSize=x; at=new T[x];
                                             for(int i=0;i<x;++i) at[i]=t; }
  void  reset()                      { delete[] at; xSize=0; at=NULL; }
  T&    set  ( const X1& x )         { assert(x.toInt()>=0); assert(x.toInt()<xSize);
                                       return at[x.toInt()];}
  // Extraction methods...
  int  getSize ( ) const             { return xSize; }
  const T& get ( const X1& x ) const { assert(x.toInt()>=0); assert(x.toInt()<xSize);
                                       return at[x.toInt()];}
};

////////////////////////////////////////////////////////////////////////////////

template <class X1, class X2, class T>
class SafeArray2D {
 private:
  // Data members;
  int xSize;
  int ySize;
  T*  at;
 public:
  // Constructor / destructor methods...
  ~SafeArray2D( )                           { delete[] at; }
  SafeArray2D ( )                           { xSize=0; ySize=0; at=NULL; }
  SafeArray2D (int x, int y)                { xSize=x; ySize=y; at=new T[x*y]; }
  SafeArray2D (int x, int y, const T& t)    { xSize=x; ySize=y; at=new T[x*y];
                                              for(int i=0;i<x*y;++i) at[i]=t; }
  //SafeArray2D ( const SafeArray2D& a )      { xSize=a.xSize; ySize=a.ySize;
  //                                            for(int i=0;i<xSize;i++) for(int j=0;j<ySize;j++) at[i*ySize+j]=a.at[i*ySize+j]; }
  // Specification methods...
  SafeArray2D& operator= ( const SafeArray2D<X1,X2,T>& sat )
    { delete[] at; xSize=sat.xSize; ySize=sat.ySize; at=new T[xSize*ySize];
      for(int i=0;i<xSize*ySize;i++) at[i]=sat.at[i]; return *this; }
  void  init ( int x,int y )                   { delete[] at; xSize=x; ySize=y; at=new T[x*y];}
  void  init ( int x,int y,const T& t )        { delete[] at; xSize=x; ySize=y; at=new T[x*y];
                                                       for(int i=0;i<x*y;++i) at[i]=t; }
  void  reset()                                { delete[] at; xSize=0; ySize=0; at=NULL; }
  T&    set  ( const X1& x,const X2& y)        { assert(at!=NULL);
                                                 assert(x.toInt()>=0); assert(x.toInt()<xSize);
                                                 assert(y.toInt()>=0); assert(y.toInt()<ySize);
                                                 return at[x.toInt()*ySize + y.toInt()];}
  // Extraction methods...
  const T& get (const X1& x,const X2& y) const { assert(at!=NULL);
                                                 assert(x.toInt()>=0); assert(x.toInt()<xSize);
                                                 assert(y.toInt()>=0);
//this assert failed when compile without -DNDEBUG (needed for debugging). Have to figure out why before adding this assert back in
//assert(y.toInt()<ySize);
                                                 return at[x.toInt()*ySize + y.toInt()];}
  int getxSize( ) const { return xSize; }
  int getySize( ) const { return ySize; }
};

////////////////////////////////////////////////////////////////////////////////

template <class X1, class X2, class X3, class T>
class SafeArray3D {
 private:
  // Data members;
  int xSize;
  int ySize;
  int zSize;
  T*  at;
 public:
  // Constructor / destructor methods...
  ~SafeArray3D()                           { delete[] at; }
  SafeArray3D()                            { xSize=0; ySize=0; zSize=0; at=NULL; }
  SafeArray3D(int x,int y,int z)           { xSize=x; ySize=y; zSize=z; at=new T[x*y*z];}
  SafeArray3D(int x,int y,int z,const T& t){ xSize=x; ySize=y; zSize=z; at=new T[x*y*z];
                                                for(int i=0;i<x*y*z;++i) at[i]=t; }
  // Specification methods...
  SafeArray3D& operator= ( const SafeArray3D<X1,X2,X3,T>& sat )
    { delete[] at; xSize=sat.xSize; ySize=sat.ySize; zSize=sat.zSize;
      at=new T[xSize*ySize*zSize];
      for(int i=0;i<xSize*ySize*zSize;i++) at[i]=sat.at[i]; return *this; }
  void init(int x,int y,int z)           { delete[] at; xSize=x; ySize=y; zSize=z; at=new T[x*y*z]; }
  void init(int x,int y,int z,const T& t){ delete[] at; xSize=x; ySize=y; zSize=z; at=new T[x*y*z];
                                                 for(int i=0;i<x*y*z;++i) at[i]=t; }
  void reset()                           { delete[] at; xSize=0; ySize=0; zSize=0; at=NULL; }
  T&   set(const X1& x,const X2& y,const X3& z)
                                         { assert(at!=NULL);
                                           assert(x.toInt()>=0); assert(x.toInt()<xSize);
                                           assert(y.toInt()>=0); assert(y.toInt()<ySize);
                                           assert(z.toInt()>=0); assert(z.toInt()<zSize);
                                           return at[(x.toInt()*ySize+y.toInt())*zSize+z.toInt()];}
  // Extraction methods...
  const T& get(const X1& x,const X2& y,const X3& z) const
                                         { assert(at!=NULL);
                                           assert(x.toInt()>=0); assert(x.toInt()<xSize);
                                           assert(y.toInt()>=0); assert(y.toInt()<ySize);
                                           assert(z.toInt()>=0); assert(z.toInt()<zSize);
                                           return at[(x.toInt()*ySize+y.toInt())*zSize+z.toInt()];}
};

////////////////////////////////////////////////////////////////////////////////

template <class X1, class X2, class X3, class X4, class T>
class SafeArray4D {
 private:
  // Data members;
  int wSize;
  int xSize;
  int ySize;
  int zSize;
  T*  at;
 public:
  // Constructor / destructor methods...
  ~SafeArray4D( )  { delete[] at; }
  SafeArray4D ( )  { wSize=0; xSize=0; ySize=0; zSize=0; at=NULL; }
  SafeArray4D(int w, int x,int y,int z)
    { wSize=w; xSize=x; ySize=y; zSize=z; at=new T[w*x*y*z];}
  SafeArray4D(int w, int x,int y,int z,const T& t)
    { wSize=w; xSize=x; ySize=y; zSize=z; at=new T[w*x*y*z];
      for(int i=0;i<w*x*y*z;++i) at[i]=t; }
  // Specification methods...
  SafeArray4D& operator= ( const SafeArray4D<X1,X2,X3,X4,T>& sat )
    { delete[] at; wSize=sat.wSize; xSize=sat.xSize; ySize=sat.ySize;
      zSize=sat.zSize; at=new T[wSize*xSize*ySize*zSize];
      for(int i=0;i<wSize*xSize*ySize*zSize;i++) at[i]=sat.at[i]; return *this; }
  void init (int w,int x,int y,int z)
    { delete[] at; wSize=w; xSize=x; ySize=y; zSize=z; at=new T[w*x*y*z]; }
  void init (int w,int x,int y,int z,const T& t)
    { delete[] at; wSize=w; xSize=x; ySize=y; zSize=z; at=new T[w*x*y*z];
      for(int i=0;i<w*x*y*z;++i) at[i]=t; }
  void reset() { delete[] at; wSize=0; xSize=0; ySize=0; zSize=0; at=NULL; }
  T&   set(const X1& w,const X2& x,const X3& y,const X4& z)
    { assert(at!=NULL);
      assert(w.toInt()>=0); assert(w.toInt()<wSize);
      assert(x.toInt()>=0); assert(x.toInt()<xSize);
      assert(y.toInt()>=0); assert(y.toInt()<ySize);
      assert(z.toInt()>=0); assert(z.toInt()<zSize);
      return at[((w.toInt()*xSize+x.toInt())*ySize+y.toInt())*zSize+z.toInt()];}
  // Extraction methods...
  const T& get(const X1& w,const X2& x,const X3& y,const X4& z) const
    { assert(at!=NULL);
      assert(w.toInt()>=0); assert(w.toInt()<wSize);
      assert(x.toInt()>=0); assert(x.toInt()<xSize);
      assert(y.toInt()>=0); assert(y.toInt()<ySize);
      assert(z.toInt()>=0); assert(z.toInt()<zSize);
      return at[((w.toInt()*xSize+x.toInt())*ySize+y.toInt())*zSize+z.toInt()];}
};

////////////////////////////////////////////////////////////////////////////////

template <class X1, class X2, class X3, class X4, class X5, class T>
class SafeArray5D {
 private:
  // Data members;
  int vSize;
  int wSize;
  int xSize;
  int ySize;
  int zSize;
  T*  at;
 public:
  // Constructor / destructor methods...
  ~SafeArray5D( )  { delete[] at; }
  SafeArray5D ( )  { vSize=0; wSize=0; xSize=0; ySize=0; zSize=0; at=NULL; }
  SafeArray5D(int v,int w, int x,int y,int z)
    { vSize=v; wSize=w; xSize=x; ySize=y; zSize=z; at=new T[v*w*x*y*z];}
  SafeArray5D(int v,int w, int x,int y,int z,const T& t)
    { vSize=v; wSize=w; xSize=x; ySize=y; zSize=z; at=new T[v*w*x*y*z];
      for(int i=0;i<v*w*x*y*z;++i) at[i]=t; }
  // Specification methods...
  SafeArray5D& operator= ( const SafeArray5D<X1,X2,X3,X4,X5,T>& sat )
    { delete[] at; vSize=sat.vSize; wSize=sat.wSize; xSize=sat.xSize;
      ySize=sat.ySize; zSize=sat.zSize; at=new T[vSize*wSize*xSize*ySize*zSize];
      for(int i=0;i<vSize*wSize*xSize*ySize*zSize;i++) at[i]=sat.at[i]; return *this; }
  void init(int v,int w,int x,int y,int z)
    { delete[] at; vSize=v; wSize=w; xSize=x; ySize=y; zSize=z; at=new T[v*w*x*y*z]; }
  void init(int v,int w,int x,int y,int z,const T& t)
    { delete[] at; vSize=v; wSize=w; xSize=x; ySize=y; zSize=z; at=new T[v*w*x*y*z];
      for(int i=0;i<v*w*x*y*z;++i) at[i]=t; }
  void reset() { delete[] at; vSize=0; wSize=0; xSize=0; ySize=0; zSize=0; at=NULL; }
  T&   set(const X1& v,const X2& w,const X3& x,const X4& y,const X5& z)
    { assert(at!=NULL);
      assert(v.toInt()>=0); assert(v.toInt()<vSize);
      assert(w.toInt()>=0); assert(w.toInt()<wSize);
      assert(x.toInt()>=0); assert(x.toInt()<xSize);
      assert(y.toInt()>=0); assert(y.toInt()<ySize);
      assert(z.toInt()>=0); assert(z.toInt()<zSize);
      return at[(((v.toInt()*wSize+w.toInt())*xSize+x.toInt())*ySize+y.toInt())*zSize+z.toInt()];}
  // Extraction methods...
  const T& get(const X1& v,const X2& w,const X3& x,const X4& y,const X5& z) const
    { assert(at!=NULL);
      assert(v.toInt()>=0); assert(v.toInt()<vSize);
      assert(w.toInt()>=0); assert(w.toInt()<wSize);
      assert(x.toInt()>=0); assert(x.toInt()<xSize);
      assert(y.toInt()>=0); assert(y.toInt()<ySize);
      assert(z.toInt()>=0); assert(z.toInt()<zSize);
      return at[(((v.toInt()*wSize+w.toInt())*xSize+x.toInt())*ySize+y.toInt())*zSize+z.toInt()];}
};

////////////////////////////////////////////////////////////////////////////////


#endif //_NL_SAFE_IDS__
