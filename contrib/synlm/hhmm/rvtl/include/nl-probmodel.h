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

#ifndef _NL_PROBMODEL__
#define _NL_PROBMODEL__

#include "nl-prob.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  XDModel
//
////////////////////////////////////////////////////////////////////////////////

template<class Y,class P> 
class Generic1DModel {
 public:
  typedef Y  RVType;
  typedef P  ProbType;
  bool setFirst ( const Y& ) const { assert(false); return false; }   // if you need it, you have to write it
  bool setNext  ( const Y& ) const { assert(false); return false; }   // if you need it, you have to write it
  P    getProb  ( const Y& ) const { assert(false); return 0.0; }     // if you need it, you have to write it
};

////////////////////////////////////////////////////////////

template<class Y,class X1,class P> 
class Generic2DModel {
 public:
  typedef Y  RVType;
  typedef X1 Dep1Type;
  typedef P  ProbType;
  bool setFirst ( const Y& ) const { assert(false); return false; }   // if you need it, you have to write it
  bool setNext  ( const Y& ) const { assert(false); return false; }   // if you need it, you have to write it
  bool setFirst ( const Y&, const X1& x1 ) const { assert(false); return false; }   // if you need it, you have to write it
  bool setNext  ( const Y&, const X1& x1 ) const { assert(false); return false; }   // if you need it, you have to write it
  P    getProb  ( const Y&, const X1& x1 ) const { assert(false); return 0.0; }     // if you need it, you have to write it
};

////////////////////////////////////////////////////////////

template<class Y,class X1,class X2,class P> 
class Generic3DModel {
 public:
  typedef Y  RVType;
  typedef X1 Dep1Type;
  typedef X2 Dep2Type;
  typedef P  ProbType;
  bool setFirst ( const Y& ) const { assert(false); return false; }   // if you need it, you have to write it
  bool setNext  ( const Y& ) const { assert(false); return false; }   // if you need it, you have to write it
  bool setFirst ( const Y&, const X1& x1, const X2& x2 ) const { assert(false); return false; }   // if you need it, you have to write it
  bool setNext  ( const Y&, const X1& x1, const X2& x2 ) const { assert(false); return false; }   // if you need it, you have to write it
  P    getProb  ( const Y&, const X1& x1, const X2& x2 ) const { assert(false); return 0.0; }     // if you need it, you have to write it
};

////////////////////////////////////////////////////////////

template<class Y,class X1,class X2,class X3,class P> 
class Generic4DModel {
 public:
  typedef Y  RVType;
  typedef X1 Dep1Type;
  typedef X2 Dep2Type;
  typedef X3 Dep3Type;
  typedef P  ProbType;
  bool setFirst ( const Y& ) const { assert(false); return false; }
  bool setNext  ( const Y& ) const { assert(false); return false; }
  bool setFirst ( const Y&, const X1& x1, const X2& x2, const X3& x3 ) const { assert(false); return false; }
  bool setNext  ( const Y&, const X1& x1, const X2& x2, const X3& x3 ) const { assert(false); return false; }
  P    getProb  ( const Y&, const X1& x1, const X2& x2, const X3& x3 ) const { assert(false); return 0.0; }
};

////////////////////////////////////////////////////////////

template<class Y,class X1,class X2,class X3,class X4,class P> 
class Generic5DModel {
 public:
  typedef Y  RVType;
  typedef X1 Dep1Type;
  typedef X2 Dep2Type;
  typedef X3 Dep3Type;
  typedef X4 Dep4Type;
  typedef P  ProbType;
  bool setFirst ( const Y& ) const { assert(false); return false; }
  bool setNext  ( const Y& ) const { assert(false); return false; }
  bool setFirst ( const Y&, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const { assert(false); return false; }
  bool setNext  ( const Y&, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const { assert(false); return false; }
  P    getProb  ( const Y&, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const { assert(false); return 0.0; }
};

////////////////////////////////////////////////////////////

template<class Y,class X1,class X2,class X3,class X4,class X5,class P> 
class Generic6DModel {
 public:
  typedef Y  RVType;
  typedef X1 Dep1Type;
  typedef X2 Dep2Type;
  typedef X3 Dep3Type;
  typedef X4 Dep4Type;
  typedef X5 Dep5Type;
  typedef P  ProbType;
  bool setFirst ( const Y& ) const { assert(false); return false; }
  bool setNext  ( const Y& ) const { assert(false); return false; }
  bool setFirst ( const Y&, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const { assert(false); return false; }
  bool setNext  ( const Y&, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const { assert(false); return false; }
  P    getProb  ( const Y&, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const { assert(false); return 0.0; }
};

////////////////////////////////////////////////////////////

template<class Y,class X1,class X2,class X3,class X4,class X5,class X6,class P> 
class Generic7DModel {
 public:
  typedef Y  RVType;
  typedef X1 Dep1Type;
  typedef X2 Dep2Type;
  typedef X3 Dep3Type;
  typedef X4 Dep4Type;
  typedef X5 Dep5Type;
  typedef X6 Dep6Type;
  typedef P  ProbType;
  bool setFirst ( const Y& ) const { assert(false); return false; }
  bool setNext  ( const Y& ) const { assert(false); return false; }
  bool setFirst ( const Y&, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, const X5& x6 ) const { assert(false); return false; }
  bool setNext  ( const Y&, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, const X5& x6 ) const { assert(false); return false; }
  P    getProb  ( const Y&, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5, const X5& x6 ) const { assert(false); return 0.0; }
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  ModeledXDRV
//
////////////////////////////////////////////////////////////////////////////////

template<class M, M& m>
class Modeled1DRV : public M::RVType {
 public:
  // Constructor / destructor methods...
  Modeled1DRV ( )                        {  }
  //Modeled1DRV ( int i )                  { M::RVType::operator=(i); }
  Modeled1DRV ( string& s )        { M::RVType::operator=(s); }
  // Static specification methods...
  static M&       setModel ( ) { return m; }
  // Static extraction methods...
  static const M& getModel ( ) { return m; }
  // Extraction methods...
  typename M::ProbType getProb ( ) const { return m.getProb(*this); }
};

////////////////////////////////////////////////////////////

template<class M, M& m>
class Modeled2DRV : public M::RVType {
 public:
  typedef M Model;
  // Constructor / destructor methods...
  Modeled2DRV ( )                                              { }
  // Modeled2DRV ( int i )                                        { M::RVType::operator=(i); }
  Modeled2DRV ( const Modeled2DRV<M,m>& mrv ) : M::RVType(mrv) { }
  Modeled2DRV ( const typename M::RVType& x )                  { M::RVType::operator=(x); }
  Modeled2DRV ( string& s )                                    { M::RVType::operator=(s); }
  // Static specification methods...
  static M&       setModel ( ) { return m; }
  // Static extraction methods...
  static const M& getModel ( ) { return m; }
  // Specification methods...
  bool setFirst           ( )                                { return M::RVType::setFirst(); }
  bool setNext            ( )                                { return M::RVType::setNext();  }
  bool setFirst           ( const typename M::Dep1Type& x1 ) { return m.setFirst(*this,x1);  }
  bool setFirstConsistent ( const typename M::Dep1Type& x1 ) { return m.setFirstConsistent(*this,x1);  }
  bool setNext            ( const typename M::Dep1Type& x1 ) { return m.setNext (*this,x1);  }
  bool setSample          ( const typename M::Dep1Type& x1 ) { return m.setSample(*this,x1); }
  // Extraction methods...
  typename M::ProbType getProb ( const typename M::Dep1Type& x1 ) const { return m.getProb(*this,x1); }
};

////////////////////////////////////////////////////////////

template<class M, M& m>
class DetermModeled2DRV : public Modeled2DRV<M,m> {
 public:
  DetermModeled2DRV ( const typename M::Dep1Type& x1 )               { m.setFirst(*this,x1); assert(LogProb(1.0)==m.getProb(*this,x1)); }
};

////////////////////////////////////////////////////////////

template<class M, M& m>
class Modeled3DRV : public M::RVType {
 public:
  // Constructor / destructor methods...
  Modeled3DRV ( )                        {  }
  //Modeled3DRV ( int i )                  { M::RVType::operator=(i); }
  Modeled3DRV ( string& s )        { M::RVType::operator=(s); }
  // Static specification methods...
  static M&       setModel ( ) { return m; }
  // Static extraction methods...
  static const M& getModel ( ) { return m; }
  // Specification methods...
  bool setFirst           ( )                                                                { return M::RVType::setFirst();    }
  bool setNext            ( )                                                                { return M::RVType::setNext();     }
  bool setFirst           ( const typename M::Dep1Type& x1, const typename M::Dep2Type& x2 ) { return m.setFirst(*this,x1,x2);  }
  bool setFirstConsistent ( const typename M::Dep1Type& x1, const typename M::Dep2Type& x2 ) { return m.setFirstConsistent(*this,x1,x2);  }
  bool setNext            ( const typename M::Dep1Type& x1, const typename M::Dep2Type& x2 ) { return m.setNext (*this,x1,x2);  }
  bool setSample          ( const typename M::Dep1Type& x1, const typename M::Dep2Type& x2 ) { return m.setSample(*this,x1,x2); }
  // Extraction methods...
  typename M::ProbType getProb ( const typename M::Dep1Type& x1,
                                 const typename M::Dep2Type& x2 ) const { return m.getProb(*this,x1,x2); }
};

////////////////////////////////////////////////////////////

template<class M, M& m>
class Modeled4DRV : public M::RVType {
 public:
  // Constructor / destructor methods...
  Modeled4DRV ( )                        {  }
  //Modeled4DRV ( int i )                  { M::RVType::operator=(i); }
  Modeled4DRV ( string& s )        { M::RVType::operator=(s); }
  // Static specification methods...
  static M&       setModel ( ) { return m; }
  // Static extraction methods...
  static const M& getModel ( ) { return m; }
  // Specification methods...
  bool setFirst           ( )                                      { return M::RVType::setFirst(); }
  bool setNext            ( )                                      { return M::RVType::setNext();  }
  bool setFirst           ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3 )       { return m.setFirst(*this,x1,x2,x3); }
  bool setFirstConsistent ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3 )       { return m.setFirstConsistent(*this,x1,x2,x3); }
  bool setNext            ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3 )       { return m.setNext (*this,x1,x2,x3); }
  bool setSample          ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3 )       { return m.setSample(*this,x1,x2,x3); }
  // Extraction methods...
  typename M::ProbType getProb ( const typename M::Dep1Type& x1,
                                 const typename M::Dep2Type& x2,
                                 const typename M::Dep3Type& x3 ) const { return m.getProb(*this,x1,x2,x3); }
};

////////////////////////////////////////////////////////////

template<class M, M& m>
class Modeled5DRV : public M::RVType {
 public:
  // Constructor / destructor methods...
  Modeled5DRV ( )                        {  }
  //Modeled5DRV ( int i )                  { M::RVType::operator=(i); }
  Modeled5DRV ( string& s )        { M::RVType::operator=(s); }
  // Static specification methods...
  static M&       setModel ( ) { return m; }
  // Static extraction methods...
  static const M& getModel ( ) { return m; }
  // Specification methods...
  bool setFirst           ( )                                      { return M::RVType::setFirst(); }
  bool setNext            ( )                                      { return M::RVType::setNext();  }
  bool setFirst           ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4 )       { return m.setFirst(*this,x1,x2,x3,x4); }
  bool setFirstConsistent ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4 )       { return m.setFirstConsistent(*this,x1,x2,x3,x4); }
  bool setNext            ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4 )       { return m.setNext (*this,x1,x2,x3,x4); }
  bool setSample           (const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4 )       { return m.setSample(*this,x1,x2,x3,x4); }
  // Extraction methods...
  typename M::ProbType getProb ( const typename M::Dep1Type& x1,
                                 const typename M::Dep2Type& x2,
                                 const typename M::Dep3Type& x3,
                                 const typename M::Dep4Type& x4 ) const { return m.getProb(*this,x1,x2,x3,x4); }
  
};

///////////////////////////////////////////////////////////////////////////////
template<class M, M& m>
class Modeled6DRV : public M::RVType {
 public:
  // Constructor / destructor methods...
  Modeled6DRV ( )                        {  }
  //Modeled5DRV ( int i )                  { M::RVType::operator=(i); }
  Modeled6DRV ( string& s )        { M::RVType::operator=(s); }
  // Static specification methods...
  static M&       setModel ( ) { return m; }
  // Static extraction methods...
  static const M& getModel ( ) { return m; }
  // Specification methods...
  bool setFirst           ( )                                      { return M::RVType::setFirst(); }
  bool setNext            ( )                                      { return M::RVType::setNext();  }
  bool setFirst           ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4,
                            const typename M::Dep5Type& x5 )       { return m.setFirst(*this,x1,x2,x3,x4,x5); }
  bool setFirstConsistent ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4,
                            const typename M::Dep5Type& x5 )       { return m.setFirstConsistent(*this,x1,x2,x3,x4,x5); }
  bool setNext            ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4,
                            const typename M::Dep5Type& x5 )       { return m.setNext (*this,x1,x2,x3,x4,x5); }
  bool setSample           (const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4,
                            const typename M::Dep5Type& x5 )       { return m.setSample(*this,x1,x2,x3,x4,x5); }
  // Extraction methods...
  typename M::ProbType getProb ( const typename M::Dep1Type& x1,
                                 const typename M::Dep2Type& x2,
                                 const typename M::Dep3Type& x3,
                                 const typename M::Dep4Type& x4,
                                 const typename M::Dep5Type& x5 ) const { return m.getProb(*this,x1,x2,x3,x4,x5); }
  
};

///////////////////////////////////////////////////////////////////////////////
template<class M, M& m>
class Modeled7DRV : public M::RVType {
 public:
  // Constructor / destructor methods...
  Modeled7DRV ( )                        {  }
  //Modeled5DRV ( int i )                  { M::RVType::operator=(i); }
  Modeled7DRV ( string& s )        { M::RVType::operator=(s); }
  // Static specification methods...
  static M&       setModel ( ) { return m; }
  // Static extraction methods...
  static const M& getModel ( ) { return m; }
  // Specification methods...
  bool setFirst           ( )                                      { return M::RVType::setFirst(); }
  bool setNext            ( )                                      { return M::RVType::setNext();  }
  bool setFirst           ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4,
                            const typename M::Dep5Type& x5,
                            const typename M::Dep6Type& x6 )       { return m.setFirst(*this,x1,x2,x3,x4,x5,x6); }
  bool setFirstConsistent ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4,
                            const typename M::Dep5Type& x5,
                            const typename M::Dep6Type& x6 )       { return m.setFirstConsistent(*this,x1,x2,x3,x4,x5,x6); }
  bool setNext            ( const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4,
                            const typename M::Dep5Type& x5,
                            const typename M::Dep6Type& x6 )       { return m.setNext (*this,x1,x2,x3,x4,x5,x6); }
  bool setSample           (const typename M::Dep1Type& x1,
                            const typename M::Dep2Type& x2,
                            const typename M::Dep3Type& x3,
                            const typename M::Dep4Type& x4,
                            const typename M::Dep5Type& x5,
                            const typename M::Dep6Type& x6 )       { return m.setSample(*this,x1,x2,x3,x4,x5,x6); }
  // Extraction methods...
  typename M::ProbType getProb ( const typename M::Dep1Type& x1,
                                 const typename M::Dep2Type& x2,
                                 const typename M::Dep3Type& x3,
                                 const typename M::Dep4Type& x4,
                                 const typename M::Dep5Type& x5,
                                 const typename M::Dep6Type& x6 ) const { return m.getProb(*this,x1,x2,x3,x4,x5,x6); }
  
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
//  PrecomputedModeledXDRV
//
////////////////////////////////////////////////////////////////////////////////

template<class M, M& m, class S>
class PrecomputedModeled2DRV : public Modeled2DRV<M,m> {
 private:
  SafeArray1D<typename M::Dep1Type,typename M::ProbType> apr;
  void precompute ( ) { apr.init(M::Dep1Type::getDomain().getSize());
                        typename M::Dep1Type x1;
                        for(bool b=x1.setFirst(); b; b=x1.setNext()) apr.set(x1)=m.getProb(*this,x1); }
 public:
  typename M::ProbType getProb ( const typename M::Dep1Type& x1 ) const { return apr.get(x1); }
  typename M::RVType   set     ( const S s ) { M::RVType::set(s); precompute(); return *this; }
};
//template<class M, M& m, class S> SafeArray1D<typename M::Dep1Type,typename M::ProbType> PrecomputedModeled2DRV<M,m,S>::apr;


#endif /* _NL_PROBMODEL__ */
