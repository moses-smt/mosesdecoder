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

#ifndef _NL_HMM_
#define _NL_HMM_

#include <list>
#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include "nl-prob.h"
#include "nl-safeids.h"
#include "nl-beam.h"
using namespace std;

typedef int Frame;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  NullBackDat - default empty back-pointer data; can replace with word or sem relation
//
////////////////////////////////////////////////////////////////////////////////

template <class MH>
class NullBackDat {
  static const string sDummy;
 public:
  NullBackDat ()             {}
  NullBackDat (const MH& mh) {}
  void write  (FILE*) const  {}
  string getString() const   { return sDummy; }
};
template <class MH>
const string NullBackDat<MH>::sDummy ( "" );


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

template <class X, class B>
class TrellNode {
 private:

  // Data members...
  Index   indSource;
  B       backptrData;
  X       xId;
  LogProb lgprMax;

 public:

  // Constructor / destructor methods...
  TrellNode ( ) { }
  TrellNode ( const Index& indS, const X& xI, const B& bDat, LogProb lgpr)
    { indSource=indS; xId=xI; lgprMax=lgpr; backptrData=bDat; /* fo = -1; */ }

  // Specification methods...
  const Index& setSource  ( ) const { return indSource; }
  const B&     setBackData( ) const { return backptrData; }
  const X&     setId      ( ) const { return xId; }
  LogProb&     setScore   ( )       { return lgprMax; }

  // Extraction methods...
  bool operator== ( const TrellNode<X,B>& tnxb ) const { return(xId==tnxb.xId); }
//  size_t       getHashKey ( ) const { return xId.getHashKey(); }
  const Index& getSource  ( ) const { return indSource; }
  const B&     getBackData( ) const { return backptrData; }
  const X&     getId      ( ) const { return xId; }
  LogProb      getLogProb ( ) const { return lgprMax; }
  LogProb      getScore   ( ) const { return lgprMax; }
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  HMM
//
////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X=typename MH::RandVarType, class B=NullBackDat<typename MH::RandVarType> >
class HMM {
 private:
  typedef std::pair<Index,B> IB;
  // Data members...
  const MH& mh;
  const MO& mo;
  SafeArray2D<Id<Frame>,Id<int>,TrellNode<X,B> > aatnTrellis;
  Frame     frameLast;
  int       iNextNode;
 public:
  // Static member varaibles...
  static bool OUTPUT_QUIET;
  static bool OUTPUT_NOISY;
  static bool OUTPUT_VERYNOISY;
  static int  BEAM_WIDTH;
  // Constructor / destructor methods...
  HMM ( const MH& mh1, const MO& mo1 ) : mh(mh1), mo(mo1) { }
  // Specification methods...
  void init         ( int, int, const X& ) ;
  void updateRanked ( const typename MO::RandVarType& ) ;
  void updateSerial ( const typename MO::RandVarType& ) ;
  void updatePara   ( const typename MO::RandVarType& ) ;
  void each         ( const typename MO::RandVarType&, Beam<LogProb,X,IB>&, SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> >& ) ;
  // Extraction methods...
  const TrellNode<X,B>& getTrellNode ( int i ) const { return aatnTrellis.get(frameLast,i); }
  int getBeamUsed ( int ) const ;
  // Input / output methods...
  void writeMLS  ( FILE* ) const ;
  void writeMLS  ( FILE*, const X& ) const ;
  void writeCurr ( FILE*, int ) const ;
  void writeCurrSum ( FILE*, int ) const ;
  void writeCurrEntropy ( FILE*, int ) const;
 //void writeCurrDepths ( FILE*, int ) const;
  void writeFoll ( FILE*, int, int, const typename MO::RandVarType& ) const ;
  void writeFollRanked ( FILE*, int, int, const typename MO::RandVarType& ) const ;
  std::list<string> getMLS() const;
  std::list<TrellNode<X,B> > getMLSnodes() const;
  std::list<string> getMLS(const X&) const;
  std::list<TrellNode<X,B> > getMLSnodes(const X&) const;
};
template <class MH, class MO, class X, class B> bool HMM<MH,MO,X,B>::OUTPUT_QUIET     = false;
template <class MH, class MO, class X, class B> bool HMM<MH,MO,X,B>::OUTPUT_NOISY     = false;
template <class MH, class MO, class X, class B> bool HMM<MH,MO,X,B>::OUTPUT_VERYNOISY = false;
template <class MH, class MO, class X, class B> int  HMM<MH,MO,X,B>::BEAM_WIDTH       = 1;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::init ( int numFr, int numX, const X& x ) {

  // Alloc trellis...
  BEAM_WIDTH = numX;
  aatnTrellis.init(numFr,BEAM_WIDTH);

  frameLast=0;

  // Set initial element at first time slice...
  aatnTrellis.set(frameLast,0) = TrellNode<X,B> ( Index(0), x, B(), 0 ) ;
}

////////////////////////////////////////////////////////////////////////////////

template <class A, class B>
bool outRank ( const quad<A,B,LogProb,Id<int> >& a1,
	       const quad<A,B,LogProb,Id<int> >& a2 ) { return (a1.third>a2.third); }

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::updateRanked ( const typename MO::RandVarType& o ) {
  // Increment frame counter...
  frameLast++;

  // Init beam for new frame...
  Beam<LogProb,X,IB> btn(BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted (BEAM_WIDTH);

  Heap < quad<int,typename MH::IterVal,LogProb,Id<int> >, outRank<int,typename MH::IterVal> > axhpiQueue;
  typedef quad<int,typename MH::IterVal,LogProb,Id<int> > XHPI;
  XHPI xhpi, xhpiTop;
  int aCtr;

  axhpiQueue.clear();
  //xhpi.first  = -1;
  //xhpi.second = HModel::IterVal();
  //xhpi.third  = 1.0;
  xhpi.first = 0;
  xhpi.third  = aatnTrellis.get(frameLast-1,xhpi.first).getScore();
  //cerr<<" xhpi not inialized yet? "<<xhpi.second<<endl;
  //xhpi.second is current hidden state h, the second parameter is hP or sP.
  xhpi.third *= mh.setIterProb ( xhpi.second, aatnTrellis.get(frameLast-1,xhpi.first).getId(), aCtr=-1 );
  //X x; mh.setTrellDat(x,xhpi.second);
  //xhpi.fourth is a
  xhpi.fourth = -1;
  //cerr<<"xhphi in the begining: "<<xhpi<<"\n";
  axhpiQueue.enqueue(xhpi);

  bool bFull=false;

  // For each ranked value of transition destination...
  for ( int iTrg=0; !bFull && axhpiQueue.getSize()>0; iTrg++ ) {
    // Iterate A* (best-first) search until a complete path is at the top of the queue...
	  //in order to make while loop stop, either axhphiQueue.getSize<0 (this is abnormal) or axhpiQueue.getTop().fourth<MH::IterVal::NUM_ITERS
	  //it seems that the second condition sohould be the normal stopping criterio. How does it happen?
	  //I know how, xhpi.fourth will say which depth of the search starts. From that state, a is 0 and decreasing to the end. above that,
	  //the value is increasing to the first variable. i.e., that is the water-edge of positive a and negative a.
   // cerr<<axhpiQueue.getTop().fourth<<" axhpiQueue.getTop().fourth vs MH::IterVal::NUM_ITERS "<<MH::IterVal::NUM_ITERS<<endl;
    while ( axhpiQueue.getSize() > 0 && axhpiQueue.getTop().fourth < MH::IterVal::NUM_ITERS ) {
      // Remove top...
      xhpiTop = axhpiQueue.dequeueTop();
      // Fork off (try to advance each elementary variable a)...
      for ( int a=xhpiTop.fourth.toInt(); a<=MH::IterVal::NUM_ITERS; a++ ) {
        // Copy top into new queue element...
        xhpi = xhpiTop;
        // At variable position -1, advance beam element for transition source...
        if ( a == -1 ) xhpi.first++;
        //cerr<<" xhpi.first in xhpi.first++: "<<xhpi.first<<" xhpi: "<<xhpi;
        // Incorporate prob from transition source...
        xhpi.third = aatnTrellis.get(frameLast-1,xhpi.first).getScore();
       // cerr<<" xhpi.third: "<<xhpi.third<<" framLast-1: "<<frameLast<<" xhpi: "<<xhpi<<endl;
        if ( xhpi.third > LogProb() ) {
          // Try to advance variable at position a and return probability (subsequent variables set to first, probability ignored)...
          xhpi.third *= mh.setIterProb ( xhpi.second, aatnTrellis.get(frameLast-1,xhpi.first).getId(), aCtr=a );
          // At end of variables, incorporate observation probability...
          if ( a == MH::IterVal::NUM_ITERS && xhpi.fourth != MH::IterVal::NUM_ITERS )
            { X x; mh.setTrellDat(x,xhpi.second); xhpi.third *= mo.getProb(o,x); }
          // Record variable position at which this element was forked off...
          //cerr<<" a after the xhpi.fourth=a: "<<a<<" xhpi.fourth: "<<xhpi.fourth<<" from partial: "<<xhpiTop<<"\n   to partial: "<<xhpi<<" NUM_ITERS: "<<MH::IterVal::NUM_ITERS<<"\n";
          xhpi.fourth = a;
          //cerr<<" a after the xhpi.fourth=a: "<<a<<" xhpi.fourth: "<<xhpi.fourth<<" from partial: "<<xhpiTop<<"\n   to partial: "<<xhpi<<" NUM_ITERS: "<<MH::IterVal::NUM_ITERS<<"\n";
          if ( xhpi.third > LogProb() ) {
            ////if ( frameLast == 4 ) cerr<<" from partial: "<<xhpiTop<<"\n   to partial: "<<xhpi<<"\n";
            // If valid, add to queue...
            axhpiQueue.enqueue(xhpi);
            //cerr<<"--------------------\n"<<axhpiQueue;
          }
        }
      }
      //the inside for loop is done here.
      // Remove top...
      //cerr<<"/-----A-----\\\n"<<axhpiQueue<<"\\-----A-----/\n";
      //if ( axhpiQueue.getTop().fourth != MH::IterVal::NUM_ITERS ) axhpiQueue.dequeueTop();
      ////cerr<<"/-----B-----\\\n"<<axhpiQueue<<"\\-----B-----/\n";
      //cerr<<axhpiQueue.getSize()<<" queue elems, "<<axhpiQueue.getTop()<<"\n";
    }
    ////cerr<<"-----*-----\n"<<axhpiQueue<<"-----*-----\n";
    //cerr<<" outside the while loop: "<<axhpiQueue.getSize()<<" queue elems **\n";
    // Add best transition (top of queue)...
    //mo.getProb(o,mh.setTrellDat(axhpiQueue.getTop().first,axhpiQueue.getTop().second));
    if ( axhpiQueue.getSize() > 0 ) {
      X x; mh.setTrellDat(x,axhpiQueue.getTop().second);
      bFull |= btn.tryAdd ( x, IB(axhpiQueue.getTop().first,mh.setBackDat(axhpiQueue.getTop().second)), axhpiQueue.getTop().third );
      //cerr<<axhpiQueue.getSize()<<" queue elems A "<<axhpiQueue.getTop()<<"\n";
      //cerr<<"/-----A-----\\\n + bFull: "<<bFull<<"\naxhpiQueue: \n"<<axhpiQueue<<"\\-----A-----/\n";
      axhpiQueue.dequeueTop();
      //cerr<<"/-----B-----\\\n"<<axhpiQueue<<"\\-----B-----/\n";
      //cerr<<axhpiQueue.getSize()<<" queue elems B "<<axhpiQueue.getTop()<<"\n";
      //cerr<<"."; cerr.flush();
    }
  }
  //the outmost for loop is over here.
  ////cerr<<"-----*-----\n"<<axhpiQueue<<"-----*-----\n";
  btn.sort(atnSorted);
  // Copy sorted beam to trellis...
  for(int i=0;i<BEAM_WIDTH;i++) {
    const std::pair<std::pair<X,IB>,LogProb>* tn1 = &atnSorted.get(i);
    //cerr<<" tn1: "<<tn1->first.second.first<<" "<<tn1->first.first<<" "<<tn1->first.second.second<<" "<<tn1->second<<endl;
    aatnTrellis.set(frameLast,i)=TrellNode<X,B>(tn1->first.second.first,
                                                tn1->first.first,
                                                tn1->first.second.second,
                                                tn1->second);
  }

  mh.update();
}


////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::updateSerial ( const typename MO::RandVarType& o ) {
  // Increment frame counter...
  frameLast++;

  // Init beam for new frame...
  Beam<LogProb,X,IB> btn(BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted (BEAM_WIDTH);

//  // Copy beam to trellis...
//  for ( int i=0; i<BEAM_WIDTH; i++ )
//    btn.set ( i, aatnTrellis.set(frameLast,i) );

  // For each possible hidden value for previous frame...
  for ( int i=0; i<BEAM_WIDTH; i++ ) {
    const TrellNode<X,B>& tnxbPrev = aatnTrellis.get(frameLast-1,i);
    X xPrev = tnxbPrev.getId();
    // If prob still not below beam minimum...
    if ( aatnTrellis.get(frameLast-1,i).getLogProb() > btn.getMin().getScore() ) {
      //if (OUTPUT_VERYNOISY) { fprintf(stderr,"FROM: "); xPrev.write(stderr); fprintf(stderr,"\n"); }
      // For each possible transition...
      typename MH::IterVal h;
      for ( bool b=mh.setFirst(h,xPrev); b; b=mh.setNext(h,xPrev) ) {
        //if (OUTPUT_VERYNOISY) { fprintf(stderr,"  TO: "); h.write(stderr); fprintf(stderr," %d*%d*%d\n",tnxbPrev.getLogProb().toInt(),lgprH.toInt(),lgprO.toInt()); }
        X x;
#ifdef O_BEFORE_H
        LogProb lgprO = mo.getProb(o,mh.setTrellDat(x,h)); if ( !OUTPUT_VERYNOISY && LogProb()==lgprO ) continue;
        LogProb lgprH = mh.getProb(h,xPrev);               if ( !OUTPUT_VERYNOISY && LogProb()==lgprH ) continue;
#else
        LogProb lgprH = mh.getProb(h,xPrev);               if ( !OUTPUT_VERYNOISY && LogProb()==lgprH ) continue;
        LogProb lgprO = mo.getProb(o,mh.setTrellDat(x,h)); if ( !OUTPUT_VERYNOISY && LogProb()==lgprO ) continue;
#endif
        LogProb lgprFull = aatnTrellis.get(frameLast-1,i).getLogProb() * lgprH * lgprO;
        if (OUTPUT_VERYNOISY) {
          cout<<"  "<<tnxbPrev.getId()<<"  ==("<<tnxbPrev.getLogProb().toInt()<<"*"<<lgprH.toInt()<<"*"<<lgprO.toInt()<<"="<<lgprFull.toInt()<<")==>  "<<h<<"\n";
          //xPrev.write(stdout);
          //fprintf(stdout,"  ==(%d*%d*%d=%d)==>  ",tnxbPrev.getLogProb().toInt(),lgprH.toInt(),lgprO.toInt(),lgprFull.toInt());
          //h.write(stdout); fprintf(stdout," "); x.write(stdout);
          //fprintf(stdout,"\n");
        }
        // Incorporate into trellis...
        btn.tryAdd ( x, IB(i,mh.setBackDat(h)), lgprFull );
        //if(OUTPUT_VERYNOISY)
        //  fprintf ( stderr,"            (X_t-1:[e^%0.6f] * H:e^%0.6f * O:e^%0.6f = X_t:[e^%0.6f])\n",
        //            float(aatnTrellis.get(frameLast-1,i).getLogProb().toInt())/100.0,
        //            float(lgprH.toInt())/100.0,
        //            float(lgprO.toInt())/100.0,
        //            float(lgprFull.toInt())/100.0 ) ;
      }
    }
  }

//  for(int i=0;i<BEAM_WIDTH;i++) {
//    fprintf(stderr,"> "); btn.get(i)->first.write(stderr); fprintf(stderr,"\n");
//  }

  btn.sort(atnSorted);

  // Copy sorted beam to trellis...
  for(int i=0;i<BEAM_WIDTH;i++) {
    const std::pair<std::pair<X,IB>,LogProb>* tn1 = &atnSorted.get(i);
    aatnTrellis.set(frameLast,i)=TrellNode<X,B>(tn1->first.second.first,
                                                tn1->first.first,
                                                tn1->first.second.second,
                                                tn1->second);
  }
}

////////////////////////////////////////////////////////////////////////////////

boost::mutex mutexHmmCtrSema;
boost::mutex mutexHmmParanoiaLock;

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::each ( const typename MO::RandVarType& o, Beam<LogProb,X,IB>& btn, SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> >& atnOut ) {

  //// for(int j=0; j<100; j+=1) for(int i=0; i<1000000; i+=1); // peg processor
//{
//  boost::mutex::scoped_lock lock1(mutexHmmParanoiaLock);

  while ( true ) {
    int i;
    bool bStop=false;
    { boost::mutex::scoped_lock lock(mutexHmmCtrSema);
      if ( (i=iNextNode) < BEAM_WIDTH ) iNextNode++;
      else bStop = true;
    } // locked to ensure no double-duty
    if ( bStop ) break;

    const TrellNode<X,B>& tnxbPrev = aatnTrellis.get(frameLast-1,i);
    // If prob still not below beam minimum...
    if ( tnxbPrev.getLogProb() > btn.getMin().getScore() ) {
      //if (OUTPUT_VERYNOISY) { fprintf(stderr,"FROM: "); tnxbPrev.getId().write(stderr); fprintf(stderr,"\n"); }

      // For each possible transition...
      const X& xPrev = tnxbPrev.getId();
      typename MH::IterVal h;
      for ( bool b=mh.setFirst(h,xPrev); b; b=mh.setNext(h,xPrev) ) {
        X x;
        LogProb lgprO;
        LogProb lgprH;
        LogProb lgprFull;
        #ifdef O_BEFORE_H //////////////////////////////////////////////////////
        lgprO = mo.getProb(o,mh.setTrellDat(x,h)); if ( !OUTPUT_VERYNOISY && LogProb()==lgprO ) continue;
        lgprH = mh.getProb(h,xPrev);               if ( !OUTPUT_VERYNOISY && LogProb()==lgprH ) continue;
        #else //////////////////////////////////////////////////////////////////
        lgprH = mh.getProb(h,xPrev);               if ( !OUTPUT_VERYNOISY && LogProb()==lgprH ) continue;
        lgprO = mo.getProb(o,mh.setTrellDat(x,h)); if ( !OUTPUT_VERYNOISY && LogProb()==lgprO ) continue;
        #endif /////////////////////////////////////////////////////////////////
        lgprFull = tnxbPrev.getLogProb() * lgprH * lgprO;
        if (OUTPUT_VERYNOISY) {
          boost::mutex::scoped_lock lock1(mutexHmmParanoiaLock);
          //fprintf(stderr,"  TO: "); h.write(stderr); fprintf(stderr,"\n");
          cout<<"  "<<tnxbPrev.getId()<<"  ==("<<tnxbPrev.getLogProb().toInt()<<"*"<<lgprH.toInt()<<"*"<<lgprO.toInt()<<"="<<lgprFull.toInt()<<")==>  "<<h<<"\n";
          //tnxbPrev.getId().write(stdout);
          //fprintf(stdout,"  ==(%d*%d*%d=%d)==>  ",tnxbPrev.getLogProb().toInt(),lgprH.toInt(),lgprO.toInt(),lgprFull.toInt());
          //h.write(stdout); fprintf(stdout," "); x.write(stdout);
          //fprintf(stdout,"\n");
        }
        // Incorporate into trellis...
        btn.tryAdd ( x, IB(i,mh.setBackDat(h)), lgprFull );
//        if(OUTPUT_VERYNOISY)
//          fprintf ( stderr,"            (X_t-1:[e^%0.6f] * H:e^%0.6f * O:e^%0.6f = X_t:[e^%0.6f])\n",
//                    float(aatnTrellis.get(frameLast-1,i).getLogProb().toInt())/100.0,
//                    float(lgprH.toInt())/100.0,
//                    float(lgprO.toInt())/100.0,
//                    float(lgprFull.toInt())/100.0 ) ;
      }
    }
  }

  if (OUTPUT_NOISY)
    { boost::mutex::scoped_lock lock1(mutexHmmParanoiaLock);
      ////    btn.sort(atnOut);
      fprintf(stdout,"========================================\n");
      for(int i=0;i<BEAM_WIDTH;i++) {
        String str; str<<"   "<<btn.get(i)->first<<":"<<btn.get(i)->second.second.first.toInt()<<":"<<btn.get(i).getScore().toInt()<<"\n";
        fprintf(stdout,str.c_array());
        //fprintf(stdout,"   "); btn.get(i)->first.write(stdout); fprintf(stdout,":%d:%d\n",btn.get(i)->second.second.first.toInt(),btn.get(i).getScore().toInt());
        ////      atnOut.get(i).first.first.write(stdout); fprintf(stdout,":%d\n",(int)atnOut.get(i).second);
      }
      fprintf(stdout,"----------------------------------------\n");
    }

//}
}

//// Something there is (about boost::bind) that doesn't love a ref.  So use a pointer for hmm and beam.
template <class MH, class MO, class X, class B>
  void global_parahmm_each ( HMM<MH,MO,X,B>* phmm, const typename MO::RandVarType* po, Beam<LogProb,X,std::pair<Index,B> >* pbtn,
                             SafeArray1D<Id<int>,std::pair<std::pair<X,std::pair<Index,B> >,LogProb> >* patn ) {
//void HMM<MH,MO,X,B>::each_static ( HMM<MH,MO,X,B>* phmm, const MO& o, Beam<LogProb,X,IB>* pbtn, SafeArray1D<int,std::pair<std::pair<X,IB>,LogProb> >* patn ) {
  phmm->each(*po,*pbtn,*patn);
}

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::updatePara ( const typename MO::RandVarType& o ) {
  // Increment frame counter...
  frameLast++;

//  fprintf(stdout,"++++%d",frameLast); o.write(stdout); fprintf(stdout,"\n");

  // Init beam for new frame...
  Beam<LogProb,X,IB>  btn1       (BEAM_WIDTH);
  Beam<LogProb,X,IB>  btn2       (BEAM_WIDTH);
  Beam<LogProb,X,IB>  btn3       (BEAM_WIDTH);
  Beam<LogProb,X,IB>  btn4       (BEAM_WIDTH);
  Beam<LogProb,X,IB>  btn5       (BEAM_WIDTH);
  Beam<LogProb,X,IB>  btn6       (BEAM_WIDTH);
  Beam<LogProb,X,IB>  btn7       (BEAM_WIDTH);
  Beam<LogProb,X,IB>  btn8       (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted1 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted2 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted3 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted4 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted5 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted6 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted7 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<X,IB>,LogProb> > atnSorted8 (BEAM_WIDTH);

  //if(0==frameLast%20)
  //  fprintf(stderr,"frame %d...\n",frameLast);
  iNextNode=0;
  boost::thread th1(boost::bind(&global_parahmm_each<MH,MO,X,B>,this,&o,&btn1,&atnSorted1));
  boost::thread th2(boost::bind(&global_parahmm_each<MH,MO,X,B>,this,&o,&btn2,&atnSorted2));
  boost::thread th3(boost::bind(&global_parahmm_each<MH,MO,X,B>,this,&o,&btn3,&atnSorted3));
  boost::thread th4(boost::bind(&global_parahmm_each<MH,MO,X,B>,this,&o,&btn4,&atnSorted4));
  boost::thread th5(boost::bind(&global_parahmm_each<MH,MO,X,B>,this,&o,&btn5,&atnSorted5));
  boost::thread th6(boost::bind(&global_parahmm_each<MH,MO,X,B>,this,&o,&btn6,&atnSorted6));
  boost::thread th7(boost::bind(&global_parahmm_each<MH,MO,X,B>,this,&o,&btn7,&atnSorted7));
  boost::thread th8(boost::bind(&global_parahmm_each<MH,MO,X,B>,this,&o,&btn8,&atnSorted8));
  th1.join();
  th2.join();
  th3.join();
  th4.join();
  th5.join();
  th6.join();
  th7.join();
  th8.join();

  for(int i=0;i<BEAM_WIDTH;i++) {
    btn1.tryAdd(btn2.get(i)->first, btn2.get(i)->second.second, btn2.get(i).getScore());
    btn1.tryAdd(btn3.get(i)->first, btn3.get(i)->second.second, btn3.get(i).getScore());
    btn1.tryAdd(btn4.get(i)->first, btn4.get(i)->second.second, btn4.get(i).getScore());
    btn1.tryAdd(btn5.get(i)->first, btn5.get(i)->second.second, btn5.get(i).getScore());
    btn1.tryAdd(btn6.get(i)->first, btn6.get(i)->second.second, btn6.get(i).getScore());
    btn1.tryAdd(btn7.get(i)->first, btn7.get(i)->second.second, btn7.get(i).getScore());
    btn1.tryAdd(btn8.get(i)->first, btn8.get(i)->second.second, btn8.get(i).getScore());
  }

  btn1.sort(atnSorted1);

  for(int i=0;i<BEAM_WIDTH;i++) {
    const std::pair<std::pair<X,IB>,LogProb>* tn1 = &atnSorted1.get(i);
    aatnTrellis.set(frameLast,i)=TrellNode<X,B>(tn1->first.second.first,
                                                tn1->first.first,
                                                tn1->first.second.second,
                                                tn1->second);
  }

//  for(int i=0;i<BEAM_WIDTH;i++) {
//    aatnTrellis.set(frameLast,i)=TrellNode<X,B>(btn1.get(i)->second.second.first,
//                                                btn1.get(i)->first,
//                                                btn1.get(i)->second.second.second,
//                                                btn1.get(i).getScore());
//  }
}


////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::writeMLS ( FILE* pf ) const {
  // Find best value at last frame...
  int     iBest = 0;
  LogProb lgprEnd ;
  for ( int i=0; i<BEAM_WIDTH; i++ )
    iBest = (aatnTrellis.get(frameLast,i).getLogProb() > aatnTrellis.get(frameLast,iBest).getLogProb()) ? i : iBest ;
  lgprEnd = aatnTrellis.get(frameLast,iBest).getLogProb();
  if(lgprEnd == LogProb()){
    fprintf(pf, "WARNING: There is no most likely sequence\n");
    return;
  }
  // Trace back most likely sequence...
  for ( Frame fr=frameLast; fr-1>=0; iBest=aatnTrellis.get(fr--,iBest).getSource().toInt() ) {
    fprintf(pf,"HYPOTH %04d> ",fr-1);
    String str; str<<aatnTrellis.get(fr,iBest).getId()<<" "<<aatnTrellis.get(fr,iBest).getBackData()<<"\n";
    fprintf(pf,str.c_array());
    //aatnTrellis.get(fr,iBest).getId().write(pf);
    //fprintf(pf," ");
    //aatnTrellis.get(fr,iBest).getBackData().write(pf);
    //fprintf(pf,"\n");
  }
  if(!OUTPUT_QUIET) fprintf(stderr,"Log prob of best path: %d\n",lgprEnd.toInt());
}

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::writeMLS ( FILE* pf, const X& xLast ) const {
  // Find best value at last frame...
  int     iBest = -1;
  LogProb lgprEnd ;
  for ( int i=0; i<BEAM_WIDTH; i++ )
    if ( xLast.compareFinal(aatnTrellis.get(frameLast,i).getId()) )
//    if ( xLast.getSub1().get(0).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(0).getSub1() &&
//         xLast.getSub1().get(1).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(1).getSub1()  )
      iBest = (-1==iBest || aatnTrellis.get(frameLast,i).getLogProb() > aatnTrellis.get(frameLast,iBest).getLogProb()) ? i : iBest ;
//      iBest = i;
  if ( -1==iBest ) iBest=0;
  lgprEnd = aatnTrellis.get(frameLast,iBest).getLogProb();
  if(lgprEnd == LogProb()){
    fprintf(pf, "WARNING: There is no most likely sequence\n");
    return;
  }
  // Trace back most likely sequence...
  for ( Frame fr=frameLast; fr-1>=0; iBest=aatnTrellis.get(fr--,iBest).getSource().toInt() ) {
    fprintf(pf,"HYPOTH %04d> ",fr-1);
    String str; str<<aatnTrellis.get(fr,iBest).getId()<<" "<<aatnTrellis.get(fr,iBest).getBackData()<<"\n";
    fprintf(pf,str.c_array());
    //aatnTrellis.get(fr,iBest).getId().write(pf);
    //fprintf(pf," ");
    //aatnTrellis.get(fr,iBest).getBackData().write(pf);
    //fprintf(pf,"\n");
  }
  if(!OUTPUT_QUIET) fprintf(stderr,"Log prob of best path: %d\n",lgprEnd.toInt());
}

////////////////////////////////////////////////////////////////////////////////

/* for getting the MLS in string form... */
template <class MH, class MO, class X, class B>
std::list<string> HMM<MH,MO,X,B>::getMLS() const {
  std::list<string> rList;
  int     iBest = 0;
  LogProb lgprEnd;
  for ( int i=0; i<BEAM_WIDTH; i++ )
    iBest = (aatnTrellis.get(frameLast,i).getLogProb() > aatnTrellis.get(frameLast,iBest).getLogProb()) ? i : iBest ;
  lgprEnd = aatnTrellis.get(frameLast,iBest).getLogProb();
  if(lgprEnd == LogProb()){
    string r = "WARNING: There is no most likely sequence\n";
    rList.push_front(r);
    return rList;
  }
  // Trace back most likely sequence...
  for ( Frame fr=frameLast; fr-1>=0; iBest=aatnTrellis.get(fr--,iBest).getSource().toInt() ) {
    char tmp[14];
    sprintf(tmp,"HYPOTH %04d> ", fr-1);
    string tString(tmp);
    tString +=  aatnTrellis.get(fr,iBest).getId().getString() + " " +
      aatnTrellis.get(fr,iBest).getBackData().getString() + "\n";
    rList.push_front(tString);
  }
  return rList;
}

/* for getting MLS in struct form... */
template <class MH, class MO, class X, class B>
std::list<TrellNode<X,B> > HMM<MH,MO,X,B>::getMLSnodes() const {
  std::list<TrellNode<X,B> > rList;
  int     iBest = 0;
  LogProb lgprEnd;
  for ( int i=0; i<BEAM_WIDTH; i++ )
    iBest = (aatnTrellis.get(frameLast,i).getLogProb() > aatnTrellis.get(frameLast,iBest).getLogProb()) ? i : iBest ;
  lgprEnd = aatnTrellis.get(frameLast,iBest).getLogProb();
  if(lgprEnd == LogProb()){
    fprintf(stderr, "WARNING: There is no most likely sequence\n");
    return rList;
  }
  // Trace back most likely sequence...
  for ( Frame fr=frameLast; fr-1>=0; iBest=aatnTrellis.get(fr--,iBest).getSource().toInt() )
    rList.push_front(aatnTrellis.get(fr,iBest));
  return rList;
}

/* for getting the MLS in string form... */
template <class MH, class MO, class X, class B>
std::list<string> HMM<MH,MO,X,B>::getMLS(const X& xLast) const {
  std::list<string> rList;
  int     iBest = -1;
  LogProb lgprEnd;
  for ( int i=0; i<BEAM_WIDTH; i++ )
    if ( xLast.compareFinal(aatnTrellis.get(frameLast,i).getId()) )
//    if ( xLast.getSub1().get(0).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(0).getSub1() &&
//         xLast.getSub1().get(1).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(1).getSub1()  )
      iBest = (-1==iBest || aatnTrellis.get(frameLast,i).getLogProb() > aatnTrellis.get(frameLast,iBest).getLogProb()) ? i : iBest ;
//      iBest = i;
  if ( -1==iBest ) iBest=0;
  lgprEnd = aatnTrellis.get(frameLast,iBest).getLogProb();
  if(lgprEnd == LogProb()){
    string r = "WARNING: There is no most likely sequence\n";
    rList.push_front(r);
    return rList;
  }
  // Trace back most likely sequence...
  for ( Frame fr=frameLast; fr-1>=0; iBest=aatnTrellis.get(fr--,iBest).getSource().toInt() ) {
////    char tmp[14];
////    sprintf(tmp,"HYPOTH %04d> ", fr-1);
////    string tString(tmp);
////    tString +=
    string tString =
////      aatnTrellis.get(fr,iBest).getId().getString() + " " +
      aatnTrellis.get(fr,iBest).getBackData().getString()
////      + "\n"
      ;
    tString = (B().getString()==tString) ? "" : " " + tString; // zero out or add space
    rList.push_front(tString);
  }
  return rList;
}

/* for getting MLS in struct form... */
template <class MH, class MO, class X, class B>
std::list<TrellNode<X,B> > HMM<MH,MO,X,B>::getMLSnodes(const X& xLast) const {
  std::list<TrellNode<X,B> > rList;
  int     iBest = -1;
  LogProb lgprEnd;
  for ( int i=0; i<BEAM_WIDTH; i++ )
    if ( xLast.compareFinal(aatnTrellis.get(frameLast,i).getId()) )
//    if ( xLast.getSub1().get(0).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(0).getSub1() &&
//         xLast.getSub1().get(1).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(1).getSub1() )
      iBest = (-1==iBest || aatnTrellis.get(frameLast,i).getLogProb() > aatnTrellis.get(frameLast,iBest).getLogProb()) ? i : iBest ;
//      iBest = i;
  if ( -1==iBest ) iBest=0;
  lgprEnd = aatnTrellis.get(frameLast,iBest).getLogProb();
  if(lgprEnd == LogProb()){
    fprintf(stderr, "WARNING: There is no most likely sequence\n");
    return rList;
  }
  // Trace back most likely sequence...
  for ( Frame fr=frameLast; fr-1>=0; iBest=aatnTrellis.get(fr--,iBest).getSource().toInt() )
    rList.push_front(aatnTrellis.get(fr,iBest));
  return rList;
}

////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::writeCurr ( FILE* pf, int f=-1 ) const {
  if ( -1==f ) f=frameLast;
  if ( 0<=f && f<=frameLast )
    for ( int i=0; i<BEAM_WIDTH; i++ )
      if(!(aatnTrellis.get(f,i).getLogProb() == LogProb())){
        fprintf(pf,"at f=%04d b=%04d: ",f,i);
        String str; str<<aatnTrellis.get(f,i).getId(); //.write(pf);
        fprintf(pf,str.c_array());
        if(f>0){
          fprintf(pf," (from ");
          String str; str<<aatnTrellis.get(f-1,aatnTrellis.get(f,i).getSource().toInt()).getId(); //.write(pf);
          fprintf(pf,str.c_array());
          fprintf(pf,")");
        }
        fprintf(pf," : e^%0.6f\n",double(aatnTrellis.get(f,i).getLogProb().toInt())/100.0);
      }
}


////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::writeCurrSum ( FILE* pf, int f=-1 ) const {
  if ( -1==f ) f=frameLast;
  if ( 0<=f && f<=frameLast ) {
    LogProb sum = 0.0;
	LogProb logtop = 0.0;
	for ( int i=0; i<BEAM_WIDTH; i++ )
      if(!(aatnTrellis.get(f,i).getLogProb() == LogProb())){
		if(i==0) { logtop=aatnTrellis.get(f,i).getLogProb(); }
		LogProb big1 = sum - logtop;
		LogProb big2 = aatnTrellis.get(f,i).getLogProb() - logtop;
        sum = LogProb( big1.toProb() + big2.toProb() ) + logtop;
      }
    fprintf(pf,"f=%04d sum=e^%0.6f\n",f,double(sum.toInt())/100.0);
  }
}


////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::writeCurrEntropy ( FILE* pf, int f=-1 ) const {
  if ( -1==f ) f=frameLast;
  if ( 0<=f && f<=frameLast ) {
    LogProb logh = 0.0;
	LogProb logtop = 0.0;
	  for ( int i=0; i<BEAM_WIDTH; i++ )
      if(!(aatnTrellis.get(f,i).getLogProb() == LogProb())){
		if(i==0) { logtop=aatnTrellis.get(f,i).getLogProb(); }
		LogProb big1 = logh - logtop;
		LogProb prob = aatnTrellis.get(f,i).getLogProb();
		LogProb big2 = prob - logtop;
		double log2prob = double(aatnTrellis.get(f,i).getLogProb().toInt()) / double(LogProb(2).toInt());
		logh = LogProb( big1.toProb() - big2.toProb()*log2prob ) + logtop;
      }
	  fprintf(pf,"f=%04d entropy=e^%0.6f\n",f,double(logh.toInt())/100.0);
  }
}


////////////////////////////////////////////////////////////////////////////////

// finds the average depth. but... depends on HHMMLangModel used.  below is for gf.
/*
template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::writeCurrDepths ( FILE* pf, int f=-1 ) const {
  if ( -1==f ) f=frameLast;
  if ( 0<=f && f<=frameLast ) {
    LogProb sum = 0.0;
	LogProb logtop = 0.0;
	Array<int> depths = Array<int>();
	Array<LogProb> logprobs = Array<LogProb>();
	double avgdepth = 0.0;
	  for ( int i=0; i<BEAM_WIDTH; i++ )
      if(!(aatnTrellis.get(f,i).getLogProb() == LogProb())){

		if(i==0) { logtop=aatnTrellis.get(f,i).getLogProb(); }
		logprobs.set(i) = aatnTrellis.get(f,i).getLogProb();

		// loop over values in S node to find lowest meaningful depth
		for ( int j=0; j<aatnTrellis.get(f,i).getId().first.getSize(); j++) {
		  // store the depth, if it's equal to G_BOT/G_BOT
		  string ac = aatnTrellis.get(f,i).getId().first.get(j).first.getString();
		  string aw = aatnTrellis.get(f,i).getId().first.get(j).second.getString();
		  depths.set(i) = 0;
		  if (ac=="-" && aw=="-") {
			//fprintf(pf,"depth at b%d = %d\n",i,j);
			depths.set(i) = j;
			break;
		  }
		}

		LogProb big1 = sum - logtop;
		LogProb big2 = aatnTrellis.get(f,i).getLogProb() - logtop;
        sum = LogProb( big1.toProb() + big2.toProb() ) + logtop;
      }

	  // hack-y stuff because the logprob sum loses prob mass.  this will give a quantized version
	  Array<LogProb> normprobs = Array<LogProb>();
	  Prob normprobsum = Prob();
	  for ( int i=0; i<BEAM_WIDTH; i++ ) {
		if(!(aatnTrellis.get(f,i).getLogProb() == LogProb())){
		  normprobs.set(i) = logprobs.get(i)/sum;
		  normprobsum += normprobs.get(i).toProb();
		}
	  }
	  for ( int i=0; i<BEAM_WIDTH; i++ ) {
		if(!(aatnTrellis.get(f,i).getLogProb() == LogProb())){
		  avgdepth += depths.get(i) * normprobs.get(i).toDouble()/normprobsum.toDouble();
		}
	  }
	  fprintf(pf,"f=%04d avg(d)=%1.6f\n",f,avgdepth);
  }
}
*/

////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::writeFollRanked ( FILE* pf, int f, int b, const typename MO::RandVarType& o ) const {
  const TrellNode<X,B>& tnxbPrev = aatnTrellis.get(f,b);
  X xPrev = tnxbPrev.getId();
  fprintf(pf,"from f=%04d b=%04d: ",f,b);
  cout<<tnxbPrev.getId()<<"\n";

  Heap < quad<int,typename MH::IterVal,LogProb,Id<int> >, outRank<int,typename MH::IterVal> > axhpiQueue;
  typedef quad<int,typename MH::IterVal,LogProb,Id<int> > XHPI;
  XHPI xhpi, xhpiTop;
  int aCtr;

  xhpi.first = b;
  xhpi.third  = tnxbPrev.getScore();
  xhpi.third *= mh.setIterProb ( xhpi.second, tnxbPrev.getId(), aCtr=-1 );
  xhpi.fourth = 0;
  axhpiQueue.enqueue(xhpi);

  //cerr<<"?? "<<xhpi<<" "<<MH::IterVal::NUM_ITERS<<"\n";

  // For each ranked value of transition destination...
  for ( int iTrg=0; iTrg<BEAM_WIDTH && axhpiQueue.getSize()>0; iTrg++ ) {
    // Iterate A* (breadth-first) search until a complete path is at the top of the queue...
    while ( axhpiQueue.getSize() > 0 && axhpiQueue.getTop().fourth < MH::IterVal::NUM_ITERS ) {
      // Remove top...
      xhpiTop = axhpiQueue.dequeueTop();
      // Fork off (try to advance each elementary variable a)...
      for ( int a=xhpiTop.fourth.toInt(); a<=MH::IterVal::NUM_ITERS; a++ ) {
	// Copy top into new queue element...
	xhpi = xhpiTop;
	// At variable position -1, advance beam element for transition source...
	if ( a == -1 ) xhpi.first++;
        // Incorporate prob from transition source...
        xhpi.third = aatnTrellis.get(f,xhpi.first).getScore();
        if ( xhpi.third > LogProb() ) {
          // Try to advance variable at position a and return probability (subsequent variables set to first, probability ignored)...
          xhpi.third *= mh.setIterProb ( xhpi.second, aatnTrellis.get(f,xhpi.first).getId(), aCtr=a );
          // At end of variables, incorporate observation probability...
          if ( a == MH::IterVal::NUM_ITERS && xhpi.fourth != MH::IterVal::NUM_ITERS )
            { X x; mh.setTrellDat(x,xhpi.second); xhpi.third *= mo.getProb(o,x); }
          // Record variable position at which this element was forked off...
          xhpi.fourth = a;
          ////cerr<<" from partial: "<<xhpiTop<<"\n   to partial: "<<xhpi<<"\n";
          if ( xhpi.third > LogProb() ) {
            cerr<<"  "<<xhpi<<"\n";
            // If valid, add to queue...
            axhpiQueue.enqueue(xhpi);
          }
        }
      }
    }
    if ( axhpiQueue.getSize() > 0 ) {
      //cerr<<axhpiQueue.getTop()<<"\n";
      axhpiQueue.dequeueTop();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X, class B>
void HMM<MH,MO,X,B>::writeFoll ( FILE* pf, int f, int b, const typename MO::RandVarType& o ) const {
  const TrellNode<X,B>& tnxbPrev = aatnTrellis.get(f,b);
  X xPrev = tnxbPrev.getId();
  fprintf(pf,"from f=%04d b=%04d: ",f,b);
  cout<<tnxbPrev.getId()<<"\n";
  // For each possible transition...
  typename MH::IterVal h;
  cout<<"HMM<MH,MO,X,B>::writeFoll OUT OF ORDER\n";
  /*
  for ( bool b=mh.setFirst(h,xPrev); b; b=mh.setNext(h,xPrev) ) {
    X x;
    LogProb lgprO = mo.getProb(o,mh.setTrellDat(x,h));
    LogProb lgprH = mh.getProb(h,xPrev);
    LogProb lgprFull = tnxbPrev.getLogProb() * lgprH * lgprO;
    cout<<"  ==("<<tnxbPrev.getLogProb().toInt()<<"*"<<lgprH.toInt()<<"*"<<lgprO.toInt()<<"="<<lgprFull.toInt()<<")==> "<<h<<"\n";
  }
  */
}

////////////////////////////////////////////////////////////////////////////////

template <class MH, class MO, class X, class B>
int HMM<MH,MO,X,B>::getBeamUsed ( int f=-1 ) const {
  if ( -1==f ) f=frameLast;
  int ctr=0;
  if ( 0<=f && f<=frameLast )
    for ( int i=0; i<BEAM_WIDTH; i++ )
      if(!(aatnTrellis.get(f,i).getLogProb() == LogProb())){
        ctr++;
      }
  return ctr;
}

#endif //_NL_HMM_

