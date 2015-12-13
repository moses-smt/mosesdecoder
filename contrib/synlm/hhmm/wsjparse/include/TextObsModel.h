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

#include "nl-cpt.h"
#include "nl-dtree.h"
#include "TextObsVars.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Models
//
////////////////////////////////////////////////////////////////////////////////


/*  WS: DO *NOT* BRING THIS BACK AND DELETE MY X MODEL !!!!!!!!!!!!!!!

//// Preterminal (POS) given constituent category models...
typedef HidVarCPT2DModel<P,C,LogProb> PgivCModel;


//// Generative model of word given tag...
class WModel {
 private:
  TrainableDTree2DModel<P,W,LogProb> modPgivWdt;

  RandAccCPT2DModel<P,W,LogProb> modPgivWs;
  RandAccCPT1DModel<P,LogProb> modP;
  RandAccCPT1DModel<W,LogProb> modW;

 public:
  //LogProb getProb ( const W& w, const HidVarCPT1DModel<P,LogProb>::IterVal& p ) const {
  LogProb getProb ( const W& w, const P::ArrayIterator<LogProb>& p ) const {
    assert(modP.getProb(p)!=LogProb());
    LogProb pr = ( (  modW.contains(w) ? modPgivWs.getProb(p,w) : modPgivWdt.getProb(p,w) )
                   * LogProb(-1000) / modP.getProb(p) );
    if(!modW.contains(w)){
    	cerr<<" w: "<<w<<" p: "<<p<<" modPgivWdt.getProb(p,w) : "<<modPgivWdt.getProb(p,w) <<endl;
    }
    return pr;
  }
  void writeFields ( FILE* pf, string sPref ) { modPgivWdt.writeFields(pf,sPref); }
  friend pair<StringInput,WModel*> operator>> ( StringInput si, WModel& m ) { return pair<StringInput,WModel*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,WModel*> delimbuff, const char* psD ) {
    StringInput si;
    return ( (si=delimbuff.first>>"W "   >>delimbuff.second->modW      >>psD)!=NULL ||
             (si=delimbuff.first>>"Pw "  >>delimbuff.second->modPgivWs >>psD)!=NULL ||
             (si=delimbuff.first>>"PwDT ">>delimbuff.second->modPgivWdt>>psD)!=NULL ||
             (si=delimbuff.first>>"P "   >>delimbuff.second->modP      >>psD)!=NULL ) ? si : StringInput(NULL);
  }
};


//// Wrapper class for model
class OModel {

 private:

  PgivCModel modPgivC;
  WModel     modWgivP;

 public:

  class DistribModeledWgivC {
   private:
    SimpleHash<C,Prob> hcpCache;
    W wW;
   public:
    DistribModeledWgivC& set ( const W& w, const OModel& m ) { wW=w; m.calcProb(*this,w); return *this; }
    void    clear   ( )                     { hcpCache.clear(); }
    Prob&   setProb ( const C& c )       { return hcpCache.set(c); }
    LogProb getProb ( const C& c ) const { return LogProb(hcpCache.get(c)); }
    W       getW    ( )            const { return wW; }
  };

  typedef DistribModeledWgivC RandVarType;



  void calcProb ( OModel::RandVarType& o, const W& w ) const {
    o.clear();
    for ( PgivCModel::const_iterator iter = modPgivC.begin(); iter!=modPgivC.end(); iter++ ) {
      C c = iter->first.getX1();
      P::ArrayIterator<LogProb> p;
      int aCtr=-1;
      //for ( bool bp=modPgivC.setIterProb(p,c,aCtr); bp; bp=modPgivC.setIterProb(p,c,aCtr=0) ) {
      for (LogProb pr=modPgivC.setIterProb(p,c,aCtr); pr!=LogProb(); pr = modPgivC.setIterProb(p,c,aCtr=0) ){
        o.setProb(c) += modPgivC.getProb(p,c).toProb() * modWgivP.getProb(w,p).toProb();
      }

    }
  }

  LogProb getProb ( const OModel::RandVarType& o, const C& c ) const { return o.getProb(c); }

  friend pair<StringInput,OModel*> operator>> ( StringInput si, OModel& m ) { return pair<StringInput,OModel*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,OModel*> delimbuff, const char* psD ) {
    StringInput si;
    return ( (si=delimbuff.first>>"Pc ">>delimbuff.second->modPgivC>>psD)!=NULL ||
             (si=delimbuff.first>>       delimbuff.second->modWgivP>>psD)!=NULL ) ? si : StringInput(NULL);
  }

  void writeFields ( FILE* pf, string sPref ) { modWgivP.writeFields(pf,sPref); }
};
*/


class XModel {

 private:

  // Data members...
  HidVarCPT2DModel<P,C,Prob>      modPgivC;
  TrainableDTree2DModel<P,X,Prob> modPgivXdt;
  RandAccCPT2DModel<P,W,Prob>     modPgivW;
  RandAccCPT1DModel<P,Prob>       modP;
  RandAccCPT1DModel<W,Prob>       modW;

 public:

  typedef X RandVarType;

  bool unknown( const X& x ) const {
    W w(x);
    return w == W_UNK;
  }

  // Extraction methods...
  LogProb getProb ( const X& x, const C& c ) const {
    Prob pr=0.0;
    W w(x);
    if ( w != W_UNK ) {
      for ( P::ArrayIterator<Prob> p = modPgivC.begin(c); !p.end(); ++p ) {
	pr += modPgivC.getProb(p,c) * modPgivW.getProb(p,w) * modW.getProb(w) / modP.getProb(p);
      }
    } else {
      for ( P::ArrayIterator<Prob> p = modPgivC.begin(c); !p.end(); ++p ) {
	pr += modPgivC.getProb(p,c) * modPgivW.getProb(p,w) * modW.getProb(w) / modP.getProb(p);
      }
    }
    return LogProb(pr);
  }

  // Input/output methods...
  friend pair<StringInput,XModel*> operator>> ( StringInput si, XModel& m ) { return pair<StringInput,XModel*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,XModel*> si_m, const char* psD ) {
    StringInput si;
    return ( (si=si_m.first>>"Pc "  >>si_m.second->modPgivC  >>psD)!=NULL ||
             (si=si_m.first>>"W "   >>si_m.second->modW      >>psD)!=NULL ||
             (si=si_m.first>>"Pw "  >>si_m.second->modPgivW  >>psD)!=NULL ||
             (si=si_m.first>>"PwDT ">>si_m.second->modPgivXdt>>psD)!=NULL ||
             (si=si_m.first>>"P "   >>si_m.second->modP      >>psD)!=NULL ) ? si : StringInput(NULL);
  }
};
