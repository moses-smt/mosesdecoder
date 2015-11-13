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
#include <iostream>
#include <iomanip>
//#include <boost/thread/thread.hpp>
//#include <boost/thread/mutex.hpp>
//#include <boost/bind.hpp>
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

template <class MY>
class NullBackDat {
  static const string sDummy;
 public:
  NullBackDat ()             {}
  NullBackDat (const MY& my) {}
  void write  (FILE*) const  {}
  string getString() const   { return sDummy; }
};
template <class MY>
const string NullBackDat<MY>::sDummy ( "" );


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
};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  HMM
//
////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S=typename MY::RandVarType, class B=NullBackDat<typename MY::RandVarType> >
class HMM {
 private:
  typedef std::pair<Index,B> IB;
  // Data members...
  const MY& my;
  const MX& mx;
  SafeArray2D<Id<Frame>,Id<int>,TrellNode<S,B> > aatnTrellis;
  Frame     frameLast;
  int       iNextNode;
 public:
  // Static member varaibles...
  static bool OUTPUT_QUIET;
  static bool OUTPUT_NOISY;
  static bool OUTPUT_VERYNOISY;
  static int  BEAM_WIDTH;
  // Constructor / destructor methods...
  HMM ( const MY& my1, const MX& mx1 ) : my(my1), mx(mx1) { }
  // Specification methods...
  void init         ( int, int, const S& ) ;
  void init          ( int, int, SafeArray1D<Id<int>,pair<S,LogProb> >* );
  void updateRanked ( const typename MX::RandVarType&, bool ) ;
  void updateSerial ( const typename MX::RandVarType& ) ;
  void updatePara   ( const typename MX::RandVarType& ) ;
  bool unknown      ( const typename MX::RandVarType& ) ;
  void each         ( const typename MX::RandVarType&, Beam<LogProb,S,IB>&, SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> >& ) ;
  // Extraction methods...
  const TrellNode<S,B>& getTrellNode ( int i ) const { return aatnTrellis.get(frameLast,i); }
  int getBeamUsed ( int ) const ;
  // Input / output methods...
  void writeMLS  ( FILE* ) const ;
  void writeMLS  ( FILE*, const S& ) const ;
  void debugPrint() const;
  double getCurrSum(int) const;
  //void writeCurr ( FILE*, int ) const ;
  void writeCurr ( ostream&, int ) const ;
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
};
template <class MY, class MX, class S, class B> bool HMM<MY,MX,S,B>::OUTPUT_QUIET     = false;
template <class MY, class MX, class S, class B> bool HMM<MY,MX,S,B>::OUTPUT_NOISY     = false;
template <class MY, class MX, class S, class B> bool HMM<MY,MX,S,B>::OUTPUT_VERYNOISY = false;
template <class MY, class MX, class S, class B> int  HMM<MY,MX,S,B>::BEAM_WIDTH       = 1;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::init ( int numFr, int numS, const S& s ) {

  // Alloc trellis...
  BEAM_WIDTH = numS;
  aatnTrellis.init(numFr,BEAM_WIDTH);

  frameLast=0;

  // Set initial element at first time slice...
  aatnTrellis.set(frameLast,0) = TrellNode<S,B> ( Index(0), s, B(), 0 ) ;
}

template <class MY, class MX, class S, class B>
  void HMM<MY,MX,S,B>::init ( int numFr, int beamSize, SafeArray1D<Id<int>,pair<S,LogProb> >* existingBeam ) {

  // Alloc trellis...
  //  int numToCopy = existingBeam->getSize();
  BEAM_WIDTH = beamSize;
  aatnTrellis.init(numFr,BEAM_WIDTH);

  frameLast=0;

  // Set initial beam elements at first time slice...
  for ( int i=0, n=existingBeam->getSize(); i<n; i++ ) {
	aatnTrellis.set(frameLast,i) = TrellNode<S,B> ( Index(0), existingBeam->get(i).first, B(), existingBeam->get(i).second ) ;
 }

}

template <class MY, class MX, class S, class B>
  void HMM<MY,MX,S,B>::debugPrint() const{

  for (int frame=0, numFrames=aatnTrellis.getxSize(); frame<numFrames; frame++) {

    for (int beamIndex=0, beamSize=aatnTrellis.getySize(); beamIndex<beamSize; beamIndex++) {

      if (aatnTrellis.get(frame,beamIndex).getLogProb().toDouble() > 0) {
	cerr << "\t" << "aatnTrellis.get(frame=" << frame << ",beamIndex=" << beamIndex << ") is\t" << aatnTrellis.get(frame,beamIndex).getId() << "\tprob=" << aatnTrellis.get(frame,beamIndex).getLogProb().toDouble() << endl;
      }

    }

  }

}

////////////////////////////////////////////////////////////////////////////////

template <class A, class B>
bool outRank ( const quad<A,B,LogProb,Id<int> >& a1,
	       const quad<A,B,LogProb,Id<int> >& a2 ) { return (a1.third>a2.third); }

template <class MY, class MX, class S, class B>
  bool HMM<MY,MX,S,B>::unknown( const typename MX::RandVarType& x ) {
  return mx.unknown(x);
}


template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::updateRanked ( const typename MX::RandVarType& x, bool b1 ) {
  // Increment frame counter...
  frameLast++;

  // Init beam for new frame...
  Beam<LogProb,S,IB> btn(BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted (BEAM_WIDTH);

  Heap < quad<int,typename MY::IterVal,LogProb,Id<int> >, outRank<int,typename MY::IterVal> > ashpiQueue;
  typedef quad<int,typename MY::IterVal,LogProb,Id<int> > SHPI;
  SHPI shpi, shpiTop;
  int aCtr;

  ashpiQueue.clear();
  //shpi.first  = -1;
  //shpi.second = HModel::IterVal();
  //shpi.third  = 1.0;
  shpi.first = 0;
  shpi.third  = aatnTrellis.get(frameLast-1,shpi.first).getScore();
  shpi.third *= my.setIterProb ( shpi.second, aatnTrellis.get(frameLast-1,shpi.first).getId(), x, b1, aCtr=-1 );
  //S s; my.setTrellDat(s,shpi.second);
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
        shpi.third = aatnTrellis.get(frameLast-1,shpi.first).getScore();
        if ( shpi.third > LogProb() ) {
          // Try to advance variable at position a and return probability (subsequent variables set to first, probability ignored)...
          shpi.third *= my.setIterProb ( shpi.second, aatnTrellis.get(frameLast-1,shpi.first).getId(), x, b1, aCtr=a );
          // At end of variables, incorporate observation probability...
          if ( a == MY::IterVal::NUM_ITERS && shpi.fourth != MY::IterVal::NUM_ITERS )
            { S s; my.setTrellDat(s,shpi.second); shpi.third *= mx.getProb(x,s); }
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
    //mx.getProb(o,my.setTrellDat(ashpiQueue.getTop().first,ashpiQueue.getTop().second));
    if ( ashpiQueue.getSize() > 0 ) {
      S s; my.setTrellDat(s,ashpiQueue.getTop().second);
      bFull |= btn.tryAdd ( s, IB(ashpiQueue.getTop().first,my.setBackDat(ashpiQueue.getTop().second)), ashpiQueue.getTop().third );
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
  for(int i=0;i<BEAM_WIDTH;i++) {
    const std::pair<std::pair<S,IB>,LogProb>* tn1 = &atnSorted.get(i);
    aatnTrellis.set(frameLast,i)=TrellNode<S,B>(tn1->first.second.first,
                                                tn1->first.first,
                                                tn1->first.second.second,
                                                tn1->second);
  }

  my.update();
}


////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::updateSerial ( const typename MX::RandVarType& x ) {
  // Increment frame counter...
  frameLast++;

  // Init beam for new frame...
  Beam<LogProb,S,IB> btn(BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted (BEAM_WIDTH);

//  // Copy beam to trellis...
//  for ( int i=0; i<BEAM_WIDTH; i++ )
//    btn.set ( i, aatnTrellis.set(frameLast,i) );

  // For each possible hidden value for previous frame...
  for ( int i=0; i<BEAM_WIDTH; i++ ) {
    const TrellNode<S,B>& tnsbPrev = aatnTrellis.get(frameLast-1,i);
    S sPrev = tnsbPrev.getId();
    // If prob still not below beam minimum...
    if ( aatnTrellis.get(frameLast-1,i).getLogProb() > btn.getMin().getScore() ) {
      //if (OUTPUT_VERYNOISY) { fprintf(stderr,"FROM: "); sPrev.write(stderr); fprintf(stderr,"\n"); }
      // For each possible transition...
      typename MY::IterVal y;
      for ( bool b=my.setFirst(y,sPrev); b; b=my.setNext(y,sPrev) ) {
        //if (OUTPUT_VERYNOISY) { fprintf(stderr,"  TO: "); y.write(stderr); fprintf(stderr," %d*%d*%d\n",tnsbPrev.getLogProb().toInt(),lgprY.toInt(),lgprO.toInt()); }
        S s;
#ifdef X_BEFORE_Y
        LogProb lgprX = mx.getProb(x,my.setTrellDat(s,y)); if ( !OUTPUT_VERYNOISY && LogProb()==lgprX ) continue;
        LogProb lgprY = my.getProb(y,sPrev);               if ( !OUTPUT_VERYNOISY && LogProb()==lgprY ) continue;
#else
        LogProb lgprY = my.getProb(y,sPrev);               if ( !OUTPUT_VERYNOISY && LogProb()==lgprY ) continue;
        LogProb lgprX = mx.getProb(x,my.setTrellDat(s,y)); if ( !OUTPUT_VERYNOISY && LogProb()==lgprX ) continue;
#endif
        LogProb lgprFull = aatnTrellis.get(frameLast-1,i).getLogProb() * lgprY * lgprX;
        if (OUTPUT_VERYNOISY) {
          cout<<"  "<<tnsbPrev.getId()<<"  ==("<<tnsbPrev.getLogProb().toInt()<<"*"<<lgprY.toInt()<<"*"<<lgprX.toInt()<<"="<<lgprFull.toInt()<<")==>  "<<y<<"\n";
          //sPrev.write(stdout);
          //fprintf(stdout,"  ==(%d*%d*%d=%d)==>  ",tnsbPrev.getLogProb().toInt(),lgprY.toInt(),lgprX.toInt(),lgprFull.toInt());
          //y.write(stdout); fprintf(stdout," "); s.write(stdout);
          //fprintf(stdout,"\n");
        }
        // Incorporate into trellis...
        btn.tryAdd ( s, IB(i,my.setBackDat(y)), lgprFull );
        //if(OUTPUT_VERYNOISY)
        //  fprintf ( stderr,"            (S_t-1:[e^%0.6f] * Y:e^%0.6f * X:e^%0.6f = S_t:[e^%0.6f])\n",
        //            float(aatnTrellis.get(frameLast-1,i).getLogProb().toInt())/100.0,
        //            float(lgprY.toInt())/100.0,
        //            float(lgprX.toInt())/100.0,
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
    const std::pair<std::pair<S,IB>,LogProb>* tn1 = &atnSorted.get(i);
    aatnTrellis.set(frameLast,i)=TrellNode<S,B>(tn1->first.second.first,
                                                tn1->first.first,
                                                tn1->first.second.second,
                                                tn1->second);
  }
}

////////////////////////////////////////////////////////////////////////////////

/*
boost::mutex mutexHmmCtrSema;
boost::mutex mutexHmmParanoiaLock;

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::each ( const typename MX::RandVarType& x, Beam<LogProb,S,IB>& btn, SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> >& atnOut ) {

  //// for(int j=0; j<100; j+=1) for(int i=0; i<1000000; i+=1); // peg processor
//{
//  boost::mutes::scoped_lock lock1(mutexHmmParanoiaLock);

  while ( true ) {
    int i;
    bool bStop=false;
    { boost::mutex::scoped_lock lock(mutexHmmCtrSema);
      if ( (i=iNextNode) < BEAM_WIDTH ) iNextNode++;
      else bStop = true;
    } // locked to ensure no double-duty
    if ( bStop ) break;

    const TrellNode<S,B>& tnsbPrev = aatnTrellis.get(frameLast-1,i);
    // If prob still not below beam minimum...
    if ( tnsbPrev.getLogProb() > btn.getMin().getScore() ) {
      //if (OUTPUT_VERYNOISY) { fprintf(stderr,"FROM: "); tnsbPrev.getId().write(stderr); fprintf(stderr,"\n"); }

      // For each possible transition...
      const S& sPrev = tnsbPrev.getId();
      typename MY::IterVal y;
      for ( bool b=my.setFirst(y,sPrev); b; b=my.setNext(y,sPrev) ) {
        S s;
        LogProb lgprX;
        LogProb lgprY;
        LogProb lgprFull;
        #ifdef X_BEFORE_Y //////////////////////////////////////////////////////
        lgprX = mx.getProb(x,my.setTrellDat(s,y)); if ( !OUTPUT_VERYNOISY && LogProb()==lgprX ) continue;
        lgprY = my.getProb(y,sPrev);               if ( !OUTPUT_VERYNOISY && LogProb()==lgprY ) continue;
        #else //////////////////////////////////////////////////////////////////
        lgprY = my.getProb(y,sPrev);               if ( !OUTPUT_VERYNOISY && LogProb()==lgprY ) continue;
        lgprX = mx.getProb(x,my.setTrellDat(s,y)); if ( !OUTPUT_VERYNOISY && LogProb()==lgprX ) continue;
        #endif /////////////////////////////////////////////////////////////////
        lgprFull = tnsbPrev.getLogProb() * lgprY * lgprX;
        if (OUTPUT_VERYNOISY) {
          boost::mutex::scoped_lock lock1(mutexHmmParanoiaLock);
          //fprintf(stderr,"  TO: "); y.write(stderr); fprintf(stderr,"\n");
          cout<<"  "<<tnsbPrev.getId()<<"  ==("<<tnsbPrev.getLogProb().toInt()<<"*"<<lgprY.toInt()<<"*"<<lgprX.toInt()<<"="<<lgprFull.toInt()<<")==>  "<<y<<"\n";
          //tnsbPrev.getId().write(stdout);
          //fprintf(stdout,"  ==(%d*%d*%d=%d)==>  ",tnsbPrev.getLogProb().toInt(),lgprY.toInt(),lgprO.toInt(),lgprFull.toInt());
          //y.write(stdout); fprintf(stdout," "); s.write(stdout);
          //fprintf(stdout,"\n");
        }
        // Incorporate into trellis...
        btn.tryAdd ( s, IB(i,my.setBackDat(y)), lgprFull );
//        if(OUTPUT_VERYNOISY)
//          fprintf ( stderr,"            (S_t-1:[e^%0.6f] * Y:e^%0.6f * X:e^%0.6f = S_t:[e^%0.6f])\n",
//                    float(aatnTrellis.get(frameLast-1,i).getLogProb().toInt())/100.0,
//                    float(lgprY.toInt())/100.0,
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
template <class MY, class MX, class S, class B>
  void global_parahmm_each ( HMM<MY,MX,S,B>* phmm, const typename MX::RandVarType* px, Beam<LogProb,S,std::pair<Index,B> >* pbtn,
                             SafeArray1D<Id<int>,std::pair<std::pair<S,std::pair<Index,B> >,LogProb> >* patn ) {
//void HMM<MY,MX,S,B>::each_static ( HMM<MY,MX,S,B>* phmm, const MX& x, Beam<LogProb,S,IB>* pbtn, SafeArray1D<int,std::pair<std::pair<S,IB>,LogProb> >* patn ) {
  phmm->each(*px,*pbtn,*patn);
}

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::updatePara ( const typename MX::RandVarType& o ) {
  // Increment frame counter...
  frameLast++;

//  fprintf(stdout,"++++%d",frameLast); o.write(stdout); fprintf(stdout,"\n");

  // Init beam for new frame...
  Beam<LogProb,S,IB>  btn1       (BEAM_WIDTH);
  Beam<LogProb,S,IB>  btn2       (BEAM_WIDTH);
  Beam<LogProb,S,IB>  btn3       (BEAM_WIDTH);
  Beam<LogProb,S,IB>  btn4       (BEAM_WIDTH);
  Beam<LogProb,S,IB>  btn5       (BEAM_WIDTH);
  Beam<LogProb,S,IB>  btn6       (BEAM_WIDTH);
  Beam<LogProb,S,IB>  btn7       (BEAM_WIDTH);
  Beam<LogProb,S,IB>  btn8       (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted1 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted2 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted3 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted4 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted5 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted6 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted7 (BEAM_WIDTH);
  SafeArray1D<Id<int>,std::pair<std::pair<S,IB>,LogProb> > atnSorted8 (BEAM_WIDTH);

  //if(0==frameLast%20)
  //  fprintf(stderr,"frame %d...\n",frameLast);
  iNextNode=0;
  boost::thread th1(boost::bind(&global_parahmm_each<MY,MX,S,B>,this,&o,&btn1,&atnSorted1));
  boost::thread th2(boost::bind(&global_parahmm_each<MY,MX,S,B>,this,&o,&btn2,&atnSorted2));
  boost::thread th3(boost::bind(&global_parahmm_each<MY,MX,S,B>,this,&o,&btn3,&atnSorted3));
  boost::thread th4(boost::bind(&global_parahmm_each<MY,MX,S,B>,this,&o,&btn4,&atnSorted4));
  boost::thread th5(boost::bind(&global_parahmm_each<MY,MX,S,B>,this,&o,&btn5,&atnSorted5));
  boost::thread th6(boost::bind(&global_parahmm_each<MY,MX,S,B>,this,&o,&btn6,&atnSorted6));
  boost::thread th7(boost::bind(&global_parahmm_each<MY,MX,S,B>,this,&o,&btn7,&atnSorted7));
  boost::thread th8(boost::bind(&global_parahmm_each<MY,MX,S,B>,this,&o,&btn8,&atnSorted8));
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
    const std::pair<std::pair<S,IB>,LogProb>* tn1 = &atnSorted1.get(i);
    aatnTrellis.set(frameLast,i)=TrellNode<S,B>(tn1->first.second.first,
                                                tn1->first.first,
                                                tn1->first.second.second,
                                                tn1->second);
  }

//  for(int i=0;i<BEAM_WIDTH;i++) {
//    aatnTrellis.set(frameLast,i)=TrellNode<S,B>(btn1.get(i)->second.second.first,
//                                                btn1.get(i)->first,
//                                                btn1.get(i)->second.second.second,
//                                                btn1.get(i).getScore());
//  }
}
*/

////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::writeMLS ( FILE* pf ) const {
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

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::writeMLS ( FILE* pf, const S& sLast ) const {
  // Find best value at last frame...
  int     iBest = -1;
  LogProb lgprEnd ;
  for ( int i=0; i<BEAM_WIDTH; i++ )
    if ( sLast.compareFinal(aatnTrellis.get(frameLast,i).getId()) )
//    if ( sLast.getSub1().get(0).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(0).getSub1() &&
//         sLast.getSub1().get(1).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(1).getSub1()  )
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
template <class MY, class MX, class S, class B>
std::list<string> HMM<MY,MX,S,B>::getMLS() const {
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
template <class MY, class MX, class S, class B>
std::list<TrellNode<S,B> > HMM<MY,MX,S,B>::getMLSnodes() const {
  std::list<TrellNode<S,B> > rList;
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
template <class MY, class MX, class S, class B>
std::list<string> HMM<MY,MX,S,B>::getMLS(const S& sLast) const {
  std::list<string> rList;
  int     iBest = -1;
  LogProb lgprEnd;
  for ( int i=0; i<BEAM_WIDTH; i++ )
    if ( sLast.compareFinal(aatnTrellis.get(frameLast,i).getId()) )
//    if ( sLast.getSub1().get(0).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(0).getSub1() &&
//         sLast.getSub1().get(1).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(1).getSub1()  )
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
template <class MY, class MX, class S, class B>
std::list<TrellNode<S,B> > HMM<MY,MX,S,B>::getMLSnodes(const S& sLast) const {
  std::list<TrellNode<S,B> > rList;
  int     iBest = -1;
  LogProb lgprEnd;
  for ( int i=0; i<BEAM_WIDTH; i++ )
    if ( sLast.compareFinal(aatnTrellis.get(frameLast,i).getId()) )
//    if ( sLast.getSub1().get(0).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(0).getSub1() &&
//         sLast.getSub1().get(1).getSub1()==aatnTrellis.get(frameLast,i).getId().getSub1().get(1).getSub1() )
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

template <class MY, class MX, class S, class B>
//void HMM<MY,MX,S,B>::writeCurr ( FILE* pf, int f=-1 ) const {
void HMM<MY,MX,S,B>::writeCurr ( ostream& os, int f=-1 ) const {
  if ( -1==f ) f=frameLast;
  if ( 0<=f && f<=frameLast )
    for ( int i=0; i<BEAM_WIDTH; i++ )
      if(!(aatnTrellis.get(f,i).getLogProb() == LogProb())){
        //fprintf(pf,"at f=%04d b=%04d: ",f,i);
        os<<"at "<<std::setfill('0')<<std::setw(4)<<f<<" "<<std::setw(4)<<i<<": ";
        //String str; str<<aatnTrellis.get(f,i).getId(); //.write(pf);
        //fprintf(pf,str.c_array());
        os<<aatnTrellis.get(f,i).getId();
        if(f>0){
          //fprintf(pf," (from ");
          //String str; str<<aatnTrellis.get(f-1,aatnTrellis.get(f,i).getSource().toInt()).getId(); //.write(pf);
          //fprintf(pf,str.c_array());
          //fprintf(pf,")");
          os<<" (from "<<aatnTrellis.get(f-1,aatnTrellis.get(f,i).getSource().toInt()).getId()<<")";
        }
        //fprintf(pf," : e^%0.6f\n",double(aatnTrellis.get(f,i).getLogProb().toInt())/100.0);
        os<<" : e^"<<(double(aatnTrellis.get(f,i).getLogProb().toInt())/100.0)<<"\n";
      }
}


////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::writeCurrSum ( FILE* pf, int f=-1 ) const {
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

template <class MY, class MX, class S, class B>
double HMM<MY,MX,S,B>::getCurrSum ( int f=-1 ) const {
  if ( -1==f ) f=frameLast;
  if ( 0<=f && f<=frameLast ) {
    LogProb negInfinity;
    double sum = 0;
    //LogProb logtop = 0;
    int sumCount=0;
    for ( int i=0; i<BEAM_WIDTH; i++ ) {
      LogProb current = aatnTrellis.get(f,i).getLogProb();
      double currentValue = current.toDouble();
      //      cerr << "Current value is " << currentValue << endl;
      //      if(!(aatnTrellis.get(f,i).getLogProb() == LogProb())){
      if (current != negInfinity) {
        if (sumCount==0) {
          sum  = currentValue;
        } else {
          sum += currentValue;
        }
        sumCount += 1;
        //		if(i==0) { logtop=aatnTrellis.get(f,i).getLogProb(); }
        //LogProb big1 = sum;// / logtop;
        // LogProb big2 = aatnTrellis.get(f,i).getLogProb();// / logtop;
        //        sum = LogProb( big1.toProb() + big2.toProb() );// * logtop;
      }
    }
    // cerr << "Summed " << sumCount << " values, total==" << sum << endl;
    return sum;// /sumCount;
  } else {
    // Should an error be thrown here instead?
    cerr << "ERROR: Invalid frame number: " << f << " of " << frameLast << endl;
    return MIN_LOG_PROB;
  }
}

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::gatherElementsInBeam( SafeArray1D<Id<int>,pair<S,LogProb> >* result, int f=-1) const {
  result->init(BEAM_WIDTH);
  if ( -1==f ) f=frameLast;
  if ( 0<=f && f<=frameLast ) {
    for ( int i=0; i<BEAM_WIDTH && &(aatnTrellis.get(f,i))!=NULL; i++ ) {
      result->set(i).first  = aatnTrellis.get(f,i).getId();
      result->set(i).second = aatnTrellis.get(f,i).getLogProb();
    }
  } else {
    //TODO How should exceptional situations be handled?
    cerr << "ERROR: Invalid argument " << f << " to gatherElementsInBeam" << endl;
  }
}

////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::writeCurrEntropy ( FILE* pf, int f=-1 ) const {
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
template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::writeCurrDepths ( FILE* pf, int f=-1 ) const {
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

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::writeFollRanked ( FILE* pf, int f, int b, const typename MX::RandVarType& x, bool b1 ) const {
  const TrellNode<S,B>& tnsbPrev = aatnTrellis.get(f,b);
  S sPrev = tnsbPrev.getId();
  fprintf(pf,"from f=%04d b=%04d: ",f,b);
  cout<<tnsbPrev.getId()<<"\n";

  //// DON'T KNOW WHAT TO DO WITH B1 (FINAL EOS FLAG)

  Heap < quad<int,typename MY::IterVal,LogProb,Id<int> >, outRank<int,typename MY::IterVal> > ashpiQueue;
  typedef quad<int,typename MY::IterVal,LogProb,Id<int> > SHPI;
  SHPI shpi, shpiTop;
  int aCtr;

  shpi.first = b;
  shpi.third  = tnsbPrev.getScore();
  shpi.third *= my.setIterProb ( shpi.second, tnsbPrev.getId(), x, b1, aCtr=-1 );
  shpi.fourth = 0;
  ashpiQueue.enqueue(shpi);

  //cerr<<"?? "<<shpi<<" "<<MY::IterVal::NUM_ITERS<<"\n";

  // For each ranked value of transition destination...
  for ( int iTrg=0; iTrg<BEAM_WIDTH && ashpiQueue.getSize()>0; iTrg++ ) {
    // Iterate A* (breadth-first) search until a complete path is at the top of the queue...
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
        shpi.third = aatnTrellis.get(f,shpi.first).getScore();
        if ( shpi.third > LogProb() ) {
          // Try to advance variable at position a and return probability (subsequent variables set to first, probability ignored)...
          shpi.third *= my.setIterProb ( shpi.second, aatnTrellis.get(f,shpi.first).getId(), x, b1, aCtr=a );
          // At end of variables, incorporate observation probability...
          if ( a == MY::IterVal::NUM_ITERS && shpi.fourth != MY::IterVal::NUM_ITERS )
            { S s; my.setTrellDat(s,shpi.second); shpi.third *= mx.getProb(x,s); }
          // Record variable position at which this element was forked off...
          shpi.fourth = a;
          ////cerr<<" from partial: "<<shpiTop<<"\n   to partial: "<<shpi<<"\n";
          if ( shpi.third > LogProb() ) {
            cerr<<"  "<<shpi<<"\n";
            // If valid, add to queue...
            ashpiQueue.enqueue(shpi);
          }
        }
      }
    }
    if ( ashpiQueue.getSize() > 0 ) {
      //cerr<<ashpiQueue.getTop()<<"\n";
      ashpiQueue.dequeueTop();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S, class B>
void HMM<MY,MX,S,B>::writeFoll ( FILE* pf, int f, int b, const typename MX::RandVarType& o ) const {
  const TrellNode<S,B>& tnsbPrev = aatnTrellis.get(f,b);
  S sPrev = tnsbPrev.getId();
  fprintf(pf,"from f=%04d b=%04d: ",f,b);
  cout<<tnsbPrev.getId()<<"\n";
  // For each possible transition...
  typename MY::IterVal h;
  cout<<"HMM<MY,MX,S,B>::writeFoll OUT OF ORDER\n";
  /*
  for ( bool b=my.setFirst(h,sPrev); b; b=my.setNext(h,sPrev) ) {
    S s;
    LogProb lgprO = mx.getProb(o,my.setTrellDat(s,h));
    LogProb lgprH = my.getProb(h,sPrev);
    LogProb lgprFull = tnsbPrev.getLogProb() * lgprH * lgprO;
    cout<<"  ==("<<tnsbPrev.getLogProb().toInt()<<"*"<<lgprH.toInt()<<"*"<<lgprO.toInt()<<"="<<lgprFull.toInt()<<")==> "<<h<<"\n";
  }
  */
}

////////////////////////////////////////////////////////////////////////////////

template <class MY, class MX, class S, class B>
int HMM<MY,MX,S,B>::getBeamUsed ( int f=-1 ) const {
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

