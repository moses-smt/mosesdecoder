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

#ifndef _NL_HMMLOOP_
#define _NL_HMMLOOP_
#include <list>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include "nl-prob.h"
#include "nl-safeids.h"
#include "nl-beam.h"

typedef int Frame;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  NullBackDat - default empty back-pointer data; can replace with word or sem relation
//
////////////////////////////////////////////////////////////////////////////////

template <class Y>
class NullBackDat {
  static const string sDummy;
  char dummy_data_member_to_avoid_compile_warning;
 public:
  NullBackDat ()             { dummy_data_member_to_avoid_compile_warning=0; }
  NullBackDat (const Y& y)   { dummy_data_member_to_avoid_compile_warning=0; }
  void write  (FILE*) const  { }
  string getString() const   { return sDummy; }
  friend ostream& operator<< ( ostream& os, const NullBackDat& nb ) { return os; }
};
template <class Y>
const string NullBackDat<Y>::sDummy ( "" );


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  Index - pointer to source in previous beam heap
//
////////////////////////////////////////////////////////////////////////////////

class Index : public Id<int> {
 public:
  Index             ( )     { }
  Index             (int i) {set(i);}
  Index& operator++ ( )     {set(toInt()+1); return *this;}
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  TrellNode - node in viterbi trellis
//
////////////////////////////////////////////////////////////////////////////////

template <class S, class B>
class TrellNode {
 private:

  // Data members...
  Index   indSource;
  B       backptrData;
  S       sId;
  LogProb lgprMax;

 public:

  // Constructor / destructor methods...
  TrellNode ( ) { }
  TrellNode ( const Index& indS, const S& sI, const B& bDat, LogProb lgpr)
    { indSource=indS; sId=sI; lgprMax=lgpr; backptrData=bDat; /* fo = -1; */ }

  // Specification methods...
  const Index& setSource  ( ) const { return indSource; }
  const B&     setBackData( ) const { return backptrData; }
  const S&     setId      ( ) const { return sId; }
  LogProb&     setScore   ( )       { return lgprMax; }

  // Extraction methods...
  bool operator== ( const TrellNode<S,B>& tnsb ) const { return(sId==tnsb.sId); }
//  size_t       getHashKey ( ) const { return sId.getHashKey(); }
  const Index& getSource  ( ) const { return indSource; }
  const B&     getBackData( ) const { return backptrData; }
  const S&     getId      ( ) const { return sId; }
  LogProb      getLogProb ( ) const { return lgprMax; }
  LogProb      getScore   ( ) const { return lgprMax; }

  // Input / output methods...
  friend ostream& operator<< ( ostream& os, const TrellNode& tn ) { return os<<tn.indSource<<","<<tn.backptrData<<","<<tn.sId<<","<<tn.lgprMax; }
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  HMMLoop
//
////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S=typename MY::RandVarType, class B=NullBackDat<typename MY::RandVarType> >
class HMMLoop {
 private:
  typedef std::pair<Index,B> IB;
  // Data members...
  MY modY;
  MX modX;
  SafeArray2D<Id<Frame>,Id<int>,TrellNode<S,B> > aatnTrellis;
  const int BEAM_WIDTH, LOOP_LENGTH;
  Frame  frameLast;
  int    iNextNode;
 public:
  // Static member varaibles...
  static bool OUTPUT_QUIET;
  static bool OUTPUT_NOISY;
  static bool OUTPUT_VERYNOISY;
//  static int  BEAM_WIDTH;
  // Constructor / destructor methods...
  HMMLoop ( int, const char*[], int, int, const S& ) ;
  // Specification methods...
//  void init         ( int, int, const S& ) ;
//  void init         ( int, int, SafeArray1D<Id<int>,pair<S,LogProb> >* );
  const TrellNode<S,B>& update       ( const typename MX::RandVarType& ) ;
  const TrellNode<S,B>& getTrellNode ( Frame t, Index i ) { return aatnTrellis.get(t%LOOP_LENGTH,i); }
  TrellNode<S,B>&       setTrellNode ( Frame t, Index i ) { return aatnTrellis.set(t%LOOP_LENGTH,i); }

 /*
  void updateSerial ( const typename MX::RandVarType& ) ;
  void updatePara   ( const typename MX::RandVarType& ) ;
  void each         ( const typename MX::RandVarType&, Beam<LogProb,S,IB>&, SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> >& ) ;
  // Extraction methods...
  const TrellNode<S,B>& getTrellNode ( int i ) const { return aatnTrellis.get(frameLast,i); }
  int getBeamUsed ( int ) const ;
  // Input / output methods...
  void writeMLS  ( FILE* ) const ;
  void writeMLS  ( FILE*, const S& ) const ;
  double getCurrSum(int) const;
  void writeCurr ( FILE*, int ) const ;
  void writeCurrSum ( FILE*, int ) const ;
  void gatherElementsInBeam( SafeArray1D<Id<int>,pair<S,LogProb> >* result, int f ) const;
  void writeCurrEntropy ( FILE*, int ) const;
  //void writeCurrDepths ( FILE*, int ) const;
  void writeFoll ( FILE*, int, int, const typename MX::RandVarType& ) const ;
  void writeFollRanked ( FILE*, int, int, const typename MX::RandVarType&, bool ) const ;
  std::list<string> getMLS() const;
  std::list<TrellNode<S,B> > getMLSnodes() const;
  std::list<string> getMLS(const S&) const;
  std::list<TrellNode<S,B> > getMLSnodes(const S&) const;
 */
};
template <class MY, class MX, class S, class B> bool HMMLoop<MY,MX,S,B>::OUTPUT_QUIET     = false;
template <class MY, class MX, class S, class B> bool HMMLoop<MY,MX,S,B>::OUTPUT_NOISY     = false;
template <class MY, class MX, class S, class B> bool HMMLoop<MY,MX,S,B>::OUTPUT_VERYNOISY = false;
//template <class MY, class MX, class S, class B> int  HMMLoop<MY,MX,S,B>::BEAM_WIDTH       = 1;


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S, class B>
HMMLoop<MY,MX,S,B>::HMMLoop ( int nArgs, const char* apsArgs[], int w, int l, const S& sInit ) : BEAM_WIDTH(w), LOOP_LENGTH(l) {

  // For each model file in command line arguments...
  for ( int iArg=1; iArg<nArgs; iArg++ ) {

    // Try to open model file...
    FILE* pf = fopen(apsArgs[iArg],"r");
    // Complain if can't open model file...
    if ( NULL == pf ) {
      cout<<"ERROR: can't open file '"<<apsArgs[iArg]<<"'!\n";
      cout<<"Terminating process with failure code 1.\n";
      exit(1);
    }

    // Initialize stream buffer and line number...
    IStreamSource iss(pf);
    int linenum=0;

    cout<<"Reading file '"<<apsArgs[iArg]<<"'...\n";

    // For each line of input...
    for ( IStream is(iss),is1; IStream()!=is; is=is1,iss.compress() ) {

      // Increment line number...
      linenum++;
      // Count off every 100K lines...
      if (linenum%100000==0) cout<<"  Reading line "<<linenum<<"...\n";

      // Try to read each line into each model...
      String s;
      if ( (is1=(is>>"#">>s>>"\n")) == IStream() &&
           (is1=(is>>modY>>  "\n")) == IStream() &&
           (is1=(is>>modX>>  "\n")) == IStream() &&
           (is1=(is>>s   >>  "\n")) != IStream() )
        // Complain if bad format...
        cout<<"  ERROR in '"<<apsArgs[iArg]<<"', line "<<linenum<<": can't process '"<<s<<"'!\n";
    }
    cout<<"Done reading file '"<<apsArgs[iArg]<<"'.\n";
    fclose(pf);
  }
  cout<<"Done reading all model files.\n";
  //modY.dump(cout,"Y");
  //modX.dump(cout,"X");

  // Alloc trellis...
  aatnTrellis.init(LOOP_LENGTH,BEAM_WIDTH);
  frameLast=LOOP_LENGTH;
  // Set initial element at first time slice...
  setTrellNode(frameLast,0) = TrellNode<S,B> ( Index(0), sInit, B(), 0 ) ;

  cout<<"Begin processing input...\n";
  IStreamSource iss(stdin);
  typename MX::RandVarType x;

  // For each frame...
  for ( IStream is(iss); is!=IStream(); iss.compress() ) {

//    // Show beam...
//    cout<<"-----BEAM:t="<<frameLast-LOOP_LENGTH<<"-----\n";
//    for(int i=0;i<BEAM_WIDTH;i++)
//      cout<<getTrellNode(frameLast,i)<<"\n";
//    cout<<"--------------\n";

    // Read spectrum (as frame audio)...
    is=is>>x;

//    // Show spectrum...
//    cout<<frameLast-2*LOOP_LENGTH+1<<" "<<x<<"\n";
//    // Show spectrum with bin numbers...
//    cout<<frameLast-2*LOOP_LENGTH+1;
//    for(int i=0; i<NUM_FREQUENCIES; i++)
//      cout<<((i==0)?' ':',')<<i<<":"<<x.get(i);
//    cout<<"\n";

    // Update trellis...
    const TrellNode<S,B>& tn = update(x);

    // Show recognized hidden variable values...
    cout<<frameLast-2*LOOP_LENGTH+1<<":'"<<tn<<"'\n";
    cout.flush();
  }
  cout<<"Done processing input.\n";
}


////////////////////////////////////////////////////////////////////////////////

template <class A, class B>
inline bool outRank ( const quad<A,B,LogProb,Id<int> >& a1,
                      const quad<A,B,LogProb,Id<int> >& a2 ) { return (a1.third>a2.third); }

template <class MY, class MX, class S, class B>
const TrellNode<S,B>& HMMLoop<MY,MX,S,B>::update ( const typename MX::RandVarType& x ) {

  // Increment frame counter...
  frameLast++;

  // Init beam for new frame...
  Beam<LogProb,S,IB> btn(BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted (BEAM_WIDTH);

  typedef quad<int,typename MY::IterVal,LogProb,Id<int> > SHPI;
  Heap < SHPI, outRank<int,typename MY::IterVal > > ashpiQueue;
  SHPI shpi, shpiTop;
  int aCtr;

  ashpiQueue.clear();
  //shpi.first  = -1;
  //shpi.second = YModel::IterVal();
  //shpi.third  = 1.0;
  shpi.first = 0;
  shpi.third  = getTrellNode(frameLast-1,shpi.first).getScore();
  shpi.third *= modY.setIterProb ( shpi.second, getTrellNode(frameLast-1,shpi.first).getId(), aCtr=-1 );   // , x, aCtr=-1 );
  //S s; modY.setTrellDat(s,shpi.second);
  shpi.fourth = -1;
  ////cerr<<"????? "<<shpi<<"\n";
  ashpiQueue.enqueue(shpi);

  bool bFull=false;

  // For each ranked value of transition destination...
  for ( int iTrg=0; !bFull && ashpiQueue.getSize()>0; iTrg++ ) {
    // Iterate A* (best-first) search until a complete path is at the top of the queue...
    while ( ashpiQueue.getSize() > 0 && ashpiQueue.getTop().fourth < MY::IterVal::NUM_ITERS ) {
      // Remove top...
      shpiTop = ashpiQueue.dequeueTop();
      // Fork off (try to advance each elementary variable a)...
      for ( int a=shpiTop.fourth.toInt(); a<=MY::IterVal::NUM_ITERS; a++ ) {
        // Copy top into new queue element...
        shpi = shpiTop;
        // At variable position -1, advance beam element for transition source...
        if ( a == -1 ) shpi.first++;
        // Incorporate prob from transition source...
        shpi.third = getTrellNode(frameLast-1,shpi.first).getScore();
        if ( shpi.third > LogProb() ) {
          // Try to advance variable at position a and return probability (subsequent variables set to first, probability ignored)...
          shpi.third *= modY.setIterProb ( shpi.second, getTrellNode(frameLast-1,shpi.first).getId(), aCtr=a );   // , x, aCtr=a );
          // At end of variables, incorporate observation probability...
          if ( a == MY::IterVal::NUM_ITERS && shpi.fourth != MY::IterVal::NUM_ITERS )
            shpi.third *= modX.getProb ( x, S(shpi.second) );
            //// { S s; modY.setTrellDat(s,shpi.second); shpi.third *= modX.getProb(x,s); }
          // Record variable position at which this element was forked off...
          shpi.fourth = a;
          //cerr<<" from partial: "<<shpiTop<<"\n   to partial: "<<shpi<<"\n";
          if ( shpi.third > LogProb() ) {
            ////if ( frameLast == 4 ) cerr<<" from partial: "<<shpiTop<<"\n   to partial: "<<shpi<<"\n";
            // If valid, add to queue...
            ashpiQueue.enqueue(shpi);
            //cerr<<"--------------------\n"<<ashpiQueue;
          }
        }
      }
      // Remove top...
      //cerr<<"/-----A-----\\\n"<<ashpiQueue<<"\\-----A-----/\n";
      //if ( ashpiQueue.getTop().fourth != MY::IterVal::NUM_ITERS ) ashpiQueue.dequeueTop();
      ////cerr<<"/-----B-----\\\n"<<ashpiQueue<<"\\-----B-----/\n";
      ////cerr<<ashpiQueue.getSize()<<" queue elems, "<<ashpiQueue.getTop()<<"\n";
    }

    ////cerr<<"-----*-----\n"<<ashpiQueue<<"-----*-----\n";
    ////cerr<<ashpiQueue.getSize()<<" queue elems **\n";

    // Add best transition (top of queue)...
    //modX.getProb(o,modY.setTrellDat(ashpiQueue.getTop().first,ashpiQueue.getTop().second));
    if ( ashpiQueue.getSize() > 0 ) {
      S s ( ashpiQueue.getTop().second );
      ////S s; modY.setTrellDat(s,ashpiQueue.getTop().second); 
      bFull |= btn.tryAdd ( s, IB(ashpiQueue.getTop().first,B(ashpiQueue.getTop().second)), ashpiQueue.getTop().third );
      ////cerr<<ashpiQueue.getSize()<<" queue elems A "<<ashpiQueue.getTop()<<"\n";
      ////cerr<<"/-----A-----\\\n"<<ashpiQueue<<"\\-----A-----/\n";
      ashpiQueue.dequeueTop();
      ////cerr<<"/-----B-----\\\n"<<ashpiQueue<<"\\-----B-----/\n";
      ////cerr<<ashpiQueue.getSize()<<" queue elems B "<<ashpiQueue.getTop()<<"\n";
      //cerr<<"."; cerr.flush();
    }
  }

  ////cerr<<"-----*-----\n"<<ashpiQueue<<"-----*-----\n";

  btn.sort(atnSorted);

  // Copy sorted beam to trellis...
  Index iOriginOfBest;
  int j=0;
  for(int i=0;i<BEAM_WIDTH;i++) {
    const std::pair<std::pair<S,IB>,LogProb>* tn1 = &atnSorted.get(i);
    Index iOrigin = tn1->first.second.first;
    // Determine origin at beginning of loop...
    for ( Frame t=frameLast-1; t>frameLast-LOOP_LENGTH+1; t-- )
      iOrigin = getTrellNode(t,iOrigin).getSource();
    if ( 0 == i ) iOriginOfBest = iOrigin;
    // If new hypothesis has same origin, add to beam...
    if ( iOriginOfBest == iOrigin ) {
      setTrellNode(frameLast,j++)=TrellNode<S,B>(tn1->first.second.first,
                                                 tn1->first.first,
                                                 tn1->first.second.second,
                                                 tn1->second);
    }
  }
  // Clear out rest of beam...
  for ( ; j<BEAM_WIDTH; j++ )
    setTrellNode(frameLast,j) = TrellNode<S,B>();

  ////modY.update();

  return getTrellNode(frameLast-LOOP_LENGTH+1,iOriginOfBest);
}




#endif //_NL_HMMLOOP_

