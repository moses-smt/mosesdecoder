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

#ifndef _NL_MINHEAP_
#define _NL_MINHEAP_

#include "nl-safeids.h"

////////////////////////////////////////////////////////////////////////////////

template <class R, bool outrank(const R&, const R&)>
class Heap {
 private:
  Array<R> at;
  int      iNextToFill;
  //SafeArray1D<Id<int>,R> at;
  // Private specification methods...
  int      heapify ( unsigned int ) ;
 public:
  // Constructor / destructor methods...
  Heap<R,outrank> ( )                   : at(10),  iNextToFill(0) { }
  Heap<R,outrank> ( int i )             : at(i),   iNextToFill(0) { }
  Heap<R,outrank> ( int i, const R& r ) : at(i,r), iNextToFill(0) { }
  // Specification methods...
  void     init       ( int i )                { iNextToFill=0; at.init(i); }
  void     clear      ( )                      { iNextToFill=0; }
  unsigned int fixIncRank ( unsigned int i );
  unsigned int fixDecRank ( unsigned int i );
  R&       set        ( unsigned int i )       { return at.set(i-1); }
  void     enqueue    ( const R& r )           { set(iNextToFill+1)=r; fixIncRank(iNextToFill+1); iNextToFill++; }
  R        dequeueTop ( )                      { R r=get(1); iNextToFill--; set(1)=get(iNextToFill+1); set(iNextToFill+1)=R(); fixDecRank(1); return r; }
  ////R&       set        ( const Id<int>& i )       { return at.set(i); }
  R&       setTop     ( )                      { return at.set(1-1); }
  // Extraction methods...
  int      getSize    ( ) const                { return iNextToFill; }
  const R& getTop     ( ) const                { return at.get(1-1); }
  const R& get        ( unsigned int i ) const { return at.get(i-1); }
  ////const R& get        ( const Id<int>& i ) const { return at.get(i); }
  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const Heap<R,outrank>& h ) { for(int i=0;i<h.iNextToFill;i++) os<<h.at.get(i)<<"\n"; return os; }
};

////////////////////////////////////////////////////////////////////////////////

template <class R, bool outrank(const R&, const R&)>
int Heap<R,outrank>::heapify ( unsigned int ind ) {
  // Find best of parent, left child, right child...
  unsigned int indBest = ind;
  indBest = (ind*2 <= (unsigned int)iNextToFill &&
	     outrank(get(ind*2),get(indBest)))
            ? ind*2 : indBest;
  indBest = (ind*2+1 <= (unsigned int)iNextToFill &&
	     outrank(get(ind*2+1),get(indBest)))
	    ? ind*2+1 : indBest;

  // If parent isn't best, restore heap property...
  if ( indBest != ind ) {
    // Swap heap elements...
    R rTemp      = get(ind);
    set(ind)     = get(indBest);
    set(indBest) = rTemp;
    // Recurse...
    return heapify(indBest);
  }
  else return ind;
}

template <class R, bool outrank(const R&, const R&)>
unsigned int Heap<R,outrank>::fixIncRank ( unsigned int ind ) {     //const R& rec ) {
  // If child outranks parent, restore heap property...
  if ( outrank(get(ind),get((ind==1)?1:ind/2)) ) {
    // Swap heap elements...
    R rTemp               = get((ind==1)?1:ind/2);
    set((ind==1)?1:ind/2) = get(ind);
    set(ind)              = rTemp;
    // Recurse on parent...
    return fixIncRank(ind/2);
  }
  else return ind;
}

template <class R, bool outrank(const R&, const R&)>
unsigned int Heap<R,outrank>::fixDecRank ( unsigned int ind ) {     //const R& rec ) {
  return heapify(ind);
}


////////////////////////////////////////////////////////////////////////////////

template <class R>
class MinHeap {
 private:
  Array<R> at;
  //SafeArray1D<Id<int>,R> at;
  // Private specification methods...
  int      minHeapify ( unsigned int ) ;
 public:
  // Constructor / destructor methods...
  MinHeap<R> ( )                   : at(10)  { }
  MinHeap<R> ( int i )             : at(i)   { }
  MinHeap<R> ( int i, const R& r ) : at(i,r) { }
  // Specification methods...
  void     init       ( int i )                  { at.init(i); }
  void     clear      ( )                        { at.clear(); }
  int      fixDecr    ( int i );
  int      fixIncr    ( int i );
  R&       set        ( unsigned int i )         { return at.set(i); }
  ////R&       set        ( const Id<int>& i )       { return at.set(i); }
  R&       setMin     ( )                        { return at.set(1-1); }
  // Extraction methods...
  int      getSize    ( ) const                  { return at.getSize(); }
  const R& getMin     ( ) const                  { return at.get(1-1); }
  const R& get        ( unsigned int i ) const   { return at.get(i); }
  ////const R& get        ( const Id<int>& i ) const { return at.get(i); }
};

////////////////////////////////////////////////////////////////////////////////

template <class R>
int MinHeap<R>::minHeapify ( unsigned int ind ) {
  // Find min of parent, left child, right child...
  unsigned int indMin = ind ;
  indMin = (ind*2 <= (unsigned int)at.getSize() &&
            at.get(ind*2-1).getScore() < at.get(indMin-1).getScore())
              ? ind*2 : indMin ;
  indMin = (ind*2+1 <= (unsigned int)at.getSize() &&
            at.get(ind*2+1-1).getScore() < at.get(indMin-1).getScore())
              ? ind*2+1 :indMin;

  // If parent isn't min, restore heap property...
  if ( indMin != ind ) {
    // Swap heap elements...
    R rTemp          = at.get(ind-1);
    at.set(ind-1)    = at.get(indMin-1);
    at.set(indMin-1) = rTemp;
    // Recurse...
    return minHeapify(indMin);
  }
  else return ind;
}

template <class R>
int MinHeap<R>::fixDecr ( int i ) {     //const R& rec ) {
  // If parent isn't min, restore heap property...
  if ( at.get((i+1)/2).getScore() > at.get(i).getScore() ) {
    // Swap heap elements...
    R rTemp         = at.get((i+1)/2);
    at.set((i+1)/2) = at.get(i);
    at.set(i)       = rTemp;
    // Recurse on parent...
    return fixDecr((i+1)/2);
  }
  else return i;
}

template <class R>
int MinHeap<R>::fixIncr ( int i ) {     //const R& rec ) {
  return minHeapify(i+1)-1;
}

#endif //_NL_HEAP_
