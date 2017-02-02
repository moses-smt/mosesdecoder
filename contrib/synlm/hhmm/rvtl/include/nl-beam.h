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

#ifndef _NL_BEAM__
#define _NL_BEAM__

#include "nl-heap.h"
#include "nl-hash.h"
//#include <boost/thread/thread.hpp>
//#include <boost/thread/mutex.hpp>
#include <tr1/unordered_map>

////////////////////////////////////////////////////////////////////////////////

/*
template <class R>
class SafePtr {
 private:
  R* pr;
  static R rDummy;
 public:
  SafePtr<R> ( )      : pr(NULL) { }
  SafePtr<R> ( R& r ) : pr(&r)   { }
  bool operator== ( const SafePtr<R>& spr ) const { return(pr==spr.pr); }
  bool operator!= ( const SafePtr<R>& spr ) const { return(!(pr==spr.pr)); }
  R&       set ( )       { assert(pr); return (pr!=NULL) ? *pr : rDummy; }
  const R& get ( ) const { return (pr!=NULL) ? *pr : rDummy; }
};
template <class R>
R SafePtr<R>::rDummy = R();

template <class S, class R>
class ScoredPtr : public SafePtr<R> {
 public:
  S scr;
  ScoredPtr<S,R> ( )           : SafePtr<R>() , scr()  { }
  ScoredPtr<S,R> ( S s, R& r ) : SafePtr<R>(r), scr(s) { }
  S& setScore()       { return scr; }
  S  getScore() const { return scr; }
};
*/

////////////////////////////////////////////////////////////////////////////////

template <class S, class C>
class ScoredIter : public C::iterator {
 private:
  //static C cDummy;
  S s;
 public:
  ScoredIter<S,C> ( )                                      : C::iterator(0,0), s()   { }
  ScoredIter<S,C> ( S s1, const typename C::iterator& i1 ) : C::iterator(i1),  s(s1) { }
  //ScoredIter<S,C> ( )                                      : C::iterator(cDummy.end()), s()   { }
  S& setScore()       { return s; }
  S  getScore() const { return s; }
};
//template <class S, class C> C ScoredIter<S,C>::cDummy;

////////////////////////////////////////////////////////////////////////////////

template <class S,class K,class D>
class Beam {
 public:
  typedef std::pair<int,D> ID;
  typedef std::pair<K,std::pair<int,D> > KID;
  typedef std::tr1::unordered_multimap<K,ID,SimpleHashFn<K>,SimpleHashEqual<K> > BeamMap;
  typedef MinHeap<ScoredIter<S,BeamMap> >                                        BeamHeap;
 private:
  BeamMap  mkid;
  BeamHeap hspkid;
 public:
  // Constructor methods...
  Beam<S,K,D> ( int i ) : mkid(2*i), hspkid(i) { for(int j=0;j<i;j++)set(j,K(),D(),S()); }
  // Specification methods...
  bool tryAdd (        const K&,   const D&,   const S&   ) ;
  void set    ( int i, const K& k, const D& d, const S& s ) { hspkid.set(i) = ScoredIter<S,BeamMap>(s,mkid.insert(KID(k,ID(i,d)))); }
  // Extraction methods...
  const ScoredIter<S,BeamMap>& getMin ( )       const { return hspkid.getMin(); }
  const ScoredIter<S,BeamMap>& get    ( int i ) const { return hspkid.get(i);   }
  void                         sort   ( SafeArray1D<Id<int>,std::pair<std::pair<K,D>,S> >& ) ;
  void write(FILE *pf){
/*    for (typename BeamMap::const_iterator i = mkid.begin(); i != mkid.end(); i++){
      i->first.write(pf);
      fprintf(pf, " %d ", i->second.first);
//      i->second.second.write(pf);
      fprintf(pf, "\n");
    }
*/
    for(int i=0; i<hspkid.getSize(); i++){
      fprintf(pf, "%d ", hspkid.get(i).getScore().toInt());
      hspkid.get(i)->first.write(pf);
      fprintf(pf, "\n");
    }
  }
};

template <class S,class K,class D>
bool Beam<S,K,D>::tryAdd ( const K& k, const D& d, const S& s ) {
  // If score good enough to get into beam...
  if ( s > hspkid.getMin().getScore() ) {
    typename BeamMap::const_iterator i = mkid.find(k);
    // If key in beam already...
    if ( i != mkid.end() ) {
      // If same key in beam now has better score...
      if ( s > hspkid.get(i->second.first).getScore() ) {
        // Update score (and data associated with that score)...
        hspkid.set(i->second.first).setScore() = s;
        hspkid.set(i->second.first)->second.second = d;
        // Update heap...
        int iStart = i->second.first; int iDeeper = hspkid.fixIncr(iStart);
        // Fix pointers in hash...
        for ( int j = iDeeper+1; j>=iStart+1; j/=2 ) hspkid.set(j-1)->second.first = j-1;
      }
    }
    // If x not in beam already, add...
    else {
      // Remove min from map (via pointer in heap)...
      mkid.erase ( hspkid.getMin() );
      // Insert new entry at min...
      set(0,k,d,s);
      // Update heap...
      int iStart = 0; int iDeeper = hspkid.fixIncr(iStart);
      // Fix pointers in hash...
      for ( int j = iDeeper+1; j>=iStart+1; j/=2 ) hspkid.set(j-1)->second.first = j-1;
    }
  }
  return ( LogProb() != hspkid.getMin().getScore() );  // true = beam full, false = beam still has gaps
}

template <class S,class K,class D>
void Beam<S,K,D>::sort ( SafeArray1D<Id<int>,std::pair<std::pair<K,D>,S> >& akdsOut ) {
  for ( int i=0; i<hspkid.getSize(); i++ ) {
    akdsOut.set(hspkid.getSize()-i-1).first.first  = hspkid.getMin()->first;         // copy min key to output key.
    akdsOut.set(hspkid.getSize()-i-1).first.second = hspkid.getMin()->second.second; // copy min dat to output dat.
    akdsOut.set(hspkid.getSize()-i-1).second       = hspkid.getMin().getScore();          // copy min scr to output scr.
    hspkid.setMin().setScore()                     = LogProb(1);            // get min out of the way.
    hspkid.fixIncr(0);                                                      // repair heap.
  }
}

////////////////////////////////////////////////////////////////////////////////


#endif //_NL_BEAM__
