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

#ifndef _NL_ARCHETYPESET_
#define _NL_ARCHETYPESET_

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include "nl-minheap.h"

////////////////////////////////////////////////////////////////////////////////

template <class S, class T>
class Scored : public T {
 public:
  S scr;
  Scored<S,T> ( )           : T() , scr()  { }
  Scored<S,T> ( S s )       : T() , scr(s) { }
  Scored<S,T> ( S s, T& t ) : T(t), scr(s) { }
  S& setScore()       { return scr; }
  S  getScore() const { return scr; }
};

////////////////////////////////////////////////////////////////////////////////

template<char* SD1,class I,char* SD2,class T,char* SD3>
class Numbered : public T {
 private:
  // Data members...
  I i;
 public:
  // Constructor / destructor methods...
  Numbered<SD1,I,SD2,T,SD3> ( )                                         { }
  Numbered<SD1,I,SD2,T,SD3> ( char* ps )                                { ps>>*this>>"\0"; }
  Numbered<SD1,I,SD2,T,SD3> ( const I& iA, const T& tA ) : T(tA), i(iA) { }
  // Specification methods...
  I&       setNumber ( )       { return i; }
  T&       setT      ( )       { return *this; }
  // Extraction methods...
  const I& getNumber ( ) const { return i; }
  const T& getT      ( ) const { return *this; }
  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const Numbered<SD1,I,SD2,T,SD3>& rv ) { return  os<<SD1<<rv.i<<SD2<<rv.getT()<<SD3; }
  friend String&  operator<< ( String& str, const Numbered<SD1,I,SD2,T,SD3>& rv ) { return str<<SD1<<rv.i<<SD2<<rv.getT()<<SD3; }
  friend pair<StringInput,Numbered<SD1,I,SD2,T,SD3>*> operator>> ( StringInput ps, Numbered<SD1,I,SD2,T,SD3>& rv ) { return pair<StringInput,Numbered<SD1,I,SD2,T,SD3>*>(ps,&rv); }
  friend StringInput    operator>> ( pair<StringInput,Numbered<SD1,I,SD2,T,SD3>*> delimbuff, const char* psPostDelim ) {
    return ( (SD3[0]=='\0') ? delimbuff.first>>SD1>>delimbuff.second->i>>SD2>>delimbuff.second->setT()>>psPostDelim
                            : delimbuff.first>>SD1>>delimbuff.second->i>>SD2>>delimbuff.second->setT()>>SD3>>psPostDelim );
  }
};

////////////////////////////////////////////////////////////////////////////////

template<class V>
class ArchetypeSet : public multimap<typename V::ElementType,Numbered<psX,int,psBar,V,psX> > {
 private:
  // Static data members...
  static const int FIRST_INDEX_TO_CHECK = 0;
  typedef Numbered<psX,int,psBar,V,psX> NV;
  typedef multimap<typename V::ElementType,NV> MapType;
  // Data members...
  MinHeap<Scored<typename V::ElementType,pair<int,SafePtr<const NV> > > > hsivCalc;
 public:
  ArchetypeSet<V> ( ) { }
  ArchetypeSet<V> ( const ArchetypeSet<V>& aa ) : MapType(aa) { cerr<<"\nCOPY!!!!\n\n"; }
  ArchetypeSet<V>& operator= ( const ArchetypeSet<V>& aa ) { cerr<<"\nCOPY2!!!!\n\n"; MapType::operator=(aa); return *this; }
  // Specification methods...
  void add ( const V& );
  // Extraction methods...
  bool isEmpty ( ) const { return MapType::empty(); }
  pair<typename V::ElementType,int> getDistanceOfNearest ( const V& ) const;
  void dump ( ) { for(typename MapType::const_iterator ii=MapType::begin(); ii!=MapType::end(); ii++) cerr<<ii->second<<"\n"; }
};

////////////////////
template<class V>
void ArchetypeSet<V>::add ( const V& v ) {
  //cerr<<"adding "<<v.get(FIRST_INDEX_TO_CHECK)<<" "<<MapType::size()<<" "<<v<<"\n";
  MapType::insert ( pair<typename V::ElementType,NV>(v.get(FIRST_INDEX_TO_CHECK),NV(MapType::size()+1,v) ) );
  ////cerr<<"adding "<<v.second.get(1)<<" ln"<<MapType::lower_bound(v.second.get(1))->second.lineNum.toInt()<<"\n";
}

////////////////////
template<class V>
pair<typename V::ElementType,int> ArchetypeSet<V>::getDistanceOfNearest ( const V& v ) const {
  //const Scored<typename V::ElementType,pair<int,SafePtr<const V> > > sipvDummy ( DBL_MAX );
  //MinHeap<Scored<typename V::ElementType,pair<int,SafePtr<const V> > > > hsiv ( MapType::size()+1, sipvDummy );
  MinHeap<Scored<typename V::ElementType,pair<int,SafePtr<const NV> > > >& hsiv =
    const_cast<MinHeap<Scored<typename V::ElementType,pair<int,SafePtr<const NV> > > >&> ( hsivCalc );
  hsiv.clear();

  typename MapType::const_iterator iUpper = MapType::upper_bound(v.get(FIRST_INDEX_TO_CHECK));
  typename MapType::const_iterator iLower = iUpper; if(iLower!=MapType::begin())iLower--;
  ////cerr<<"seeking "<<v.get(0)<<" (upper=ln"<<(&iUpper->second)<<" "<<((iUpper!=MapType::end())?iUpper->first:-1)<<", lower=ln"<<&iLower->second<<" "<<iLower->first<<")\n";
  int iNext = 0;
  if ( iUpper!=MapType::end() ) {
    hsiv.set(iNext).first = FIRST_INDEX_TO_CHECK;
    hsiv.set(iNext).second = SafePtr<const NV> ( iUpper->second );
    typename V::ElementType d = v.getMarginalDistance ( hsiv.get(iNext).first, hsiv.get(iNext).second.getRef() );
    hsiv.set(iNext).setScore() = d;
    //hsiv.set(iNext).setScore() = v.getMarginalDistance ( hsiv.getMin().first, iUpper->second.second );
    ////int j =
    hsiv.fixDecr(iNext);
    ////cerr<<"    adding ln"<<&hsiv.get(j).second.getRef()<<" marg-dist="<<d<<" new-score="<<double(hsiv.get(j).getScore())<<" new-pos="<<j<<"\n";
    iNext++;
    ////for(int i=0;i<iNext;i++) cerr<<"      "<<i<<": ln"<<hsiv.get(i).second.getRef().lineNum.toInt()<<" new-score="<<double(hsiv.get(i).getScore())<<"\n";
  }
  hsiv.set(iNext).first = FIRST_INDEX_TO_CHECK;
  hsiv.set(iNext).second = SafePtr<const NV> ( iLower->second );
  typename V::ElementType d = v.getMarginalDistance ( hsiv.get(iNext).first, hsiv.get(iNext).second.getRef() );
  hsiv.set(iNext).setScore() = d;
  //hsiv.set(iNext).setScore() = v.getMarginalDistance ( hsiv.getMin().first, iLower->second.second );
  ////int j =
  hsiv.fixDecr(iNext);
  ////cerr<<"    adding ln"<<&hsiv.get(j).second.getRef()<<" marg-dist="<<d<<" new-score="<<double(hsiv.get(j).getScore())<<" new-pos="<<j<<"\n";
  iNext++;
  ////for(int i=0;i<iNext;i++) cerr<<"      "<<i<<": ln"<<hsiv.get(i).second.getRef().lineNum.toInt()<<" new-score="<<double(hsiv.get(i).getScore())<<"\n";
  while ( hsiv.getMin().first < V::SIZE-1 ) {
    typename V::ElementType d = v.getMarginalDistance ( ++hsiv.setMin().first, hsiv.getMin().second.getRef() );
    hsiv.setMin().setScore() += d;
    ////cerr<<" matching ln"<<&hsiv.getMin().second.getRef()<<" i="<<hsiv.setMin().first<<" marg-dist="<<d<<" new-score="<<hsiv.getMin().getScore();
    ////int j =
    hsiv.fixIncr(0);
    ////cerr<<" new-pos="<<j<<"\n";
    ////if(j!=0) for(int i=0;i<iNext;i++) cerr<<"      "<<i<<": ln"<<hsiv.get(i).second.getRef().lineNum.toInt()<<" new-score="<<double(hsiv.get(i).getScore())<<"\n";
    if ( iUpper!=MapType::end() && &hsiv.getMin().second.getRef() == &iUpper->second ) {
      iUpper++;
      if ( iUpper!=MapType::end() ) {
        hsiv.set(iNext).first = FIRST_INDEX_TO_CHECK;
        hsiv.set(iNext).second = SafePtr<const NV> ( iUpper->second );
        typename V::ElementType d = v.getMarginalDistance ( hsiv.get(iNext).first, hsiv.get(iNext).second.getRef() );
        hsiv.set(iNext).setScore() = d;
        ////int j =
        hsiv.fixDecr(iNext);
        ////cerr<<"    adding ln"<<&hsiv.get(j).second.getRef()<<" marg-dist="<<d<<" new-score="<<double(hsiv.get(j).getScore())<<" new-pos="<<j<<"\n";
        iNext++;
        ////for(int i=0;i<iNext;i++) cerr<<"      "<<i<<": ln"<<hsiv.get(i).second.getRef().lineNum.toInt()<<" new-score="<<double(hsiv.get(i).getScore())<<"\n";
      }
    }
    if ( iLower!=MapType::begin() && &hsiv.getMin().second.getRef() == &iLower->second ) {
      iLower--;
      hsiv.set(iNext).first = FIRST_INDEX_TO_CHECK;
      hsiv.set(iNext).second = SafePtr<const NV> ( iLower->second );
      typename V::ElementType d = v.getMarginalDistance ( hsiv.get(iNext).first, hsiv.get(iNext).second.getRef() );
      hsiv.set(iNext).setScore() = d;
      ////int j =
      hsiv.fixDecr(iNext);
      ////cerr<<"    adding ln"<<&hsiv.get(j).second.getRef()<<" marg-dist="<<d<<" new-score="<<double(hsiv.get(j).getScore())<<" new-pos="<<j<<"\n";
      iNext++;
      ////for(int i=0;i<iNext;i++) cerr<<"      "<<i<<": ln"<<hsiv.get(i).second.getRef().lineNum.toInt()<<" new-score="<<double(hsiv.get(i).getScore())<<"\n";
    }
  }
  return pair<typename V::ElementType,int> ( hsiv.getMin().getScore(), hsiv.getMin().second.getRef().getNumber() );
}


////////////////////////////////////////////////////////////////////////////////

#endif //_NL_ARCHITYPESET_
