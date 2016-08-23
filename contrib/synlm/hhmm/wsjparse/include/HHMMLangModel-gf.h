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
#include "TextObsVars.h"

char psX[]="";
char psSlash[]="/";
char psComma[]=",";
char psSemi[]=";";
char psSemiSemi[]=";;";
char psDashDiamondDash[]="-<>-";
char psTilde[]="~";
//char psBar[]="|";
char psLBrace[]="{";
char psRBrace[]="}";
char psLangle[]="<";
char psRangle[]=">";
char psLbrack[]="[";
char psRbrack[]="]";

const char* BEG_STATE = "-/-;-/-;-/-;-/-;-";
const char* END_STATE = "eos/eos;-/-;-/-;-/-;-";


////////////////////////////////////////////////////////////////////////////////
//
//  Random Variables
//
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////// Simple Variables

//// B: boolean
DiscreteDomain<char> domB;
class B : public DiscreteDomainRV<char,domB> {
 public:
  B ( )                : DiscreteDomainRV<char,domB> ( )    { }
  B ( const char* ps ) : DiscreteDomainRV<char,domB> ( ps ) { }
};
const B B_0 ("0");
const B B_1 ("1");


//// D: depth (input only, to HHMM models)...
DiscreteDomain<char> domD;
class D : public DiscreteDomainRV<char,domD> {
 public:
  D ( )                : DiscreteDomainRV<char,domD> ( )    { }
  D ( int i )          : DiscreteDomainRV<char,domD> ( i )  { }
  D ( const char* ps ) : DiscreteDomainRV<char,domD> ( ps ) { }
};
const D D_0("0");
const D D_1("1");
const D D_2("2");
const D D_3("3");
const D D_4("4");
const D D_5("5");


//// G: grammatical constituent category
DiscreteDomain<int> domG;
class G : public DiscreteDomainRV<int,domG> {
 private:
  static SimpleHash<G,B> hIsTerm;
  void calcDetModels ( string s ) {
    if (!hIsTerm.contains(*this)) {
      hIsTerm.set(*this) = (('A'<=s.c_str()[0] && s.c_str()[0]<='Z') || s.find('_')!=string::npos) ? B_0 : B_1;
    }
  }
 public:
  G ( )                               : DiscreteDomainRV<int,domG> ( )    { }
  template<class P>
  G ( const G::ArrayIterator<P>& it )                                     { setVal(it); }
  G ( const char* ps )                : DiscreteDomainRV<int,domG> ( ps ) { calcDetModels(ps); }
  B isTerm ( ) const { return hIsTerm.get(*this); }
  friend pair<StringInput,G*> operator>> ( StringInput si, G& g ) { return pair<StringInput,G*>(si,&g); }
  friend StringInput operator>> ( pair<StringInput,G*> si_g, const char* psD ) {
    if ( si_g.first == NULL ) return NULL;
    StringInput si=si_g.first>>(DiscreteDomainRV<int,domG>&)*si_g.second>>psD;
    si_g.second->calcDetModels(si_g.second->getString()); return si; }
};
SimpleHash<G,B> G::hIsTerm;
const G G_NIL("-");
const G G_SUB("-");    // G_SUB = G_NIL
const G G_TOP("DISC");
const G G_RST("REST");

typedef G C;


//// A: added feature tags for underspec cats
DiscreteDomain<char> domA;
class A : public DiscreteDomainRV<char,domA> {
 public:
  A ( )                : DiscreteDomainRV<char,domA> ( )    { }
  A ( const char* ps ) : DiscreteDomainRV<char,domA> ( ps ) { }
};
const A A_NIL ("-");


//////////////////////////////////////// Formally Joint Variables Implemented as Simple Variables

//// Rd: final-state (=FGA)...
DiscreteDomain<int> domRd;
class Rd : public DiscreteDomainRV<int,domRd> {
 private:
  static SimpleHash<Rd,B> hToB;
  static SimpleHash<Rd,G> hToG;
  static SimpleHash<G,Rd> hFromG;
  void calcDetModels ( string s ) {
    if (!hToB.contains(*this)) {
      size_t i=s.find(',');
      assert(i!=string::npos);
      hToB.set(*this) = B(s.substr(0,i).c_str());
    }
    if (!hToG.contains(*this)) {
      size_t i=s.find(',');
      assert(i!=string::npos);
      hToG.set(*this) = G(s.substr(i+1).c_str());
      if ( '1'==s[0] )
        hFromG.set(G(s.substr(i+1).c_str())) = *this;
    }
  }
 public:
  Rd ( )                                       : DiscreteDomainRV<int,domRd> ( )    { }
  Rd ( const DiscreteDomainRV<int,domRd>& rv ) : DiscreteDomainRV<int,domRd> ( rv ) { }
  Rd ( const char* ps )                        : DiscreteDomainRV<int,domRd> ( ps ) { calcDetModels(ps); }
  Rd ( const G& g )                                                                 { *this = hFromG.get(g); }
  B         getB  ( )     const { return hToB.get(*this); }
  G         getG  ( )     const { return hToG.get(*this); }
  static Rd getRd ( G g )       { return hFromG.get(g); }
  friend pair<StringInput,Rd*> operator>> ( StringInput si, Rd& m ) { return pair<StringInput,Rd*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,Rd*> si_m, const char* psD ) {
    if ( si_m.first == NULL ) return NULL;
    StringInput si=si_m.first>>(DiscreteDomainRV<int,domRd>&)*si_m.second>>psD;
    si_m.second->calcDetModels(si_m.second->getString()); return si; }
};
SimpleHash<Rd,B> Rd::hToB;
SimpleHash<Rd,G> Rd::hToG;
SimpleHash<G,Rd> Rd::hFromG;
const Rd Rd_INC("0,-"); // BOT
const Rd Rd_SUB("1,-"); // TOP


//////////////////////////////////////// Formally and Implementationally Joint Variables

//// Sd: store element...
class Sd : public DelimitedJoint2DRV<psX,G,psSlash,G,psX> {
  typedef DelimitedJoint2DRV<psX,G,psSlash,G,psX> Parent;
 public:
  Sd ( )                                : Parent()        { }
  template<class P>
  Sd ( const Sd::ArrayIterator<P>& it )                   { setVal(it); }
  Sd ( const G& gia, const G& giw )   : Parent(gia,giw) { }
  const G& getAct ( ) const { return first;  }
  const G& getAwa ( ) const { return second; }
  template<class P> class ArrayIterator : public Parent::ArrayIterator<P> {
   public:
    G::ArrayIterator<P>& setAct ( )  { return Parent::ArrayIterator<P>::first;  }
    G::ArrayIterator<P>& setAwa ( )  { return Parent::ArrayIterator<P>::second; }
  };
  friend pair<StringInput,Sd*> operator>> ( StringInput si, Sd& sd ) { return pair<StringInput,Sd*>(si,&sd); }
  friend StringInput operator>> ( pair<StringInput,Sd*> si_sd, const char* psD ) {
    if ( si_sd.first == NULL ) return NULL;
    StringInput si = si_sd.first>>*(Parent*)si_sd.second>>psD;
    return si;
  }
};
const Sd Sd_TOP(G_TOP,G_RST);
const Sd Sd_SUB(G_SUB,G_SUB);


//// R: collection of syntactic variables at all depths in each `reduce' phase...
typedef DelimitedJointArrayRV<4,psSemi,Rd> R;


//// S: collection of syntactic variables at all depths in each `shift' phase...
class S : public DelimitedJoint2DRV<psX,DelimitedJointArrayRV<4,psSemi,Sd>,psSemi,G,psX> {
 public:
  operator G()  const { return ( ( (second      != G_SUB) ? second :
                                   (first.get(3)!=Sd_SUB) ? first.get(3).second :
                                   (first.get(2)!=Sd_SUB) ? first.get(2).second :
                                   (first.get(1)!=Sd_SUB) ? first.get(1).second :
                                                            first.get(0).second ) ); }
  bool compareFinal ( const S& s ) const { return(*this==s); }
};


//// Y: the set of all (marginalized) reduce and (modeled) shift variables in the HHMM...
class Y : public DelimitedJoint2DRV<psX,R,psDashDiamondDash,S,psX>
{ public:
  operator R() const {return first;}
  operator S() const {return second;}
};


////////////////////////////////////////////////////////////////////////////////
//
//  Models
//
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////// "Wrapper" models for individual RVs...

//// Model of Rd given D and Rd and Sd (from above) and Sd (from previous)
class RdModel {
 private:
  HidVarCPT4DModel<Rd,D,G,G,LogProb>        mRd;    // Reduction model: F giv D, G (active cat from prev), G (awaited cat from above) (assume prev awa = reduc)
  static const HidVarCPT1DModel<Rd,LogProb> mRd_INC; // Fixed Rd_INC model.
  static const HidVarCPT1DModel<Rd,LogProb> mRd_SUB; // Fixed Rd_SUB model.
 public:
  //static bool Rd_ROOT_OBS;
  LogProb setIterProb ( Rd::ArrayIterator<LogProb>& rd, const D& d, const Rd& rdD, const Sd& sdP, const Sd& sdU, bool b1, int& vctr ) const {
    LogProb pr;
    if ( rdD==Rd_SUB && (sdP.getAwa()==G_SUB) ) {
      // _/sub 1,sub (bottom) case...
      pr = mRd_SUB.setIterProb(rd,vctr);
    }
    else if ( rdD==Rd_SUB && sdP.getAwa().isTerm()==B_1 ) {
      // _/term 1,sub (middle) case...
      pr = mRd.setIterProb(rd,d,sdU.getAwa(),sdP.getAct(),vctr);
      if ( vctr<-1 && pr==LogProb() ) cerr<<"\nERROR: no condition F "<<d<<" "<<sdU.getAwa()<<" "<<sdP.getAct()<<"\n\n";
    }
    else {
      // _/noterm or 0,_ (top) case, otherwise...
      pr =  mRd_INC.setIterProb(rd,vctr);
    }
    // Iterate again if result doesn't match root observation...
    if ( vctr>=-1 && d==D_1 && b1!=(Rd(rd).getB()==B_1) ) pr=LogProb();
    //cerr<<"    Rd "<<d<<" "<<rdD<<" "<<sdP<<" "<<sdU<<" : "<<rd<<" = "<<pr<<" ("<<vctr<<")\n";
    return pr;
  }
  friend pair<StringInput,RdModel*> operator>> ( StringInput si, RdModel& m ) { return pair<StringInput,RdModel*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,RdModel*> si_m, const char* psD ) {
    StringInput si;
    return ( (si=si_m.first>>"F ">>si_m.second->mRd>>psD)!=NULL ) ? si : StringInput(NULL);
  }
};
const HidVarCPT1DModel<Rd,LogProb> RdModel::mRd_INC(Rd_INC);
const HidVarCPT1DModel<Rd,LogProb> RdModel::mRd_SUB(Rd_SUB);


//// Model of Sd given D and Rd and Rd and Sd(from prev) and Sd(from above)
class SdModel {
 public:
  HidVarCPT3DModel<G,D,G,LogProb>   mGe;   // Expansion model of G given D, G (from above)
 private:
  HidVarCPT4DModel<G,D,G,G,LogProb> mGtaa; // Active transition model of G (active)  given D, G (above awa), G (from reduction)
  HidVarCPT4DModel<G,D,G,G,LogProb> mGtaw; // Active transition model of G (awaited) given D, G (prev act), G (from reduction)
  HidVarCPT4DModel<G,D,G,G,LogProb> mGtww; // Awaited transition model of G (awaited) given D, G (prev awa), G (reduction below)
  static const HidVarCPT1DModel<G,LogProb>   mG_SUB; // Fixed G_SUB model
  static       HidVarCPT2DModel<G,G,LogProb> mG_CPY; // Cached G_CPY model  --  WARNING: STATIC NON-CONST is not thread safe!
 public:
  LogProb setIterProb ( Sd::ArrayIterator<LogProb>& sd, const D& d, const Rd& rdD, const Rd& rd, const Sd& sdP, const Sd& sdU, int& vctr ) const {
    LogProb pr,p;
    if (rdD.getB()!=B_0) {
      if (rd.getB()!=B_0 || rd.getG()!=G_NIL) {  //if (rd!=Rd_INC) {
        if (rd.getB()==B_1) {
          if (sdU.getAwa().isTerm()==B_1 || sdU==Sd_SUB) {
            // 1,g 1,g (expansion to sub) case:
            pr  = mG_SUB.setIterProb(sd.setAct(),vctr);
            pr *= mG_SUB.setIterProb(sd.setAwa(),vctr);
          }
          else {
            // 1,g 1,g (expansion) case:
            pr  = p = mGe.setIterProb(sd.setAct() ,d,sdU.getAwa(),vctr);
            if ( vctr<-1 && p==LogProb() ) cerr<<"\nERROR: no condition Ge "<<d<<" "<<sdU.getAwa()<<"\n\n";
            if ( !mG_CPY.contains(G(sd.setAct())) ) mG_CPY.setProb(G(sd.setAct()),G(sd.setAct()))=1.0;
            pr *= p = mG_CPY.setIterProb(sd.setAwa(),G(sd.setAct()),vctr);
          }
        }
        else {
          // 1,_ 0,g (active transition following reduction) case:
          pr  = p = mGtaa.setIterProb(sd.setAct(),d,sdU.getAwa(),rd.getG(),vctr);
          if ( vctr<-1 && p==LogProb() ) cerr<<"\nERROR: no condition Gtaa "<<d<<" "<<sdU.getAwa()<<" "<<rd.getG()<<" ("<<rd<<")\n\n";
          pr *= p = mGtaw.setIterProb(sd.setAwa(),d,G(sd.setAct()),rd.getG(),vctr);
          if ( vctr<-1 && p==LogProb() ) cerr<<"\nERROR: no condition Gtaw "<<d<<" "<<G(sd.setAct())<<" "<<rd.getG()<<"\n\n";
        }
      }
      else {
        // 1,g 0,- (awaited transition without reduction) case:
        if ( !mG_CPY.contains(sdP.getAct()) ) mG_CPY.setProb(sdP.getAct(),sdP.getAct())=1.0;
        pr  = p = mG_CPY.setIterProb(sd.setAct(),sdP.getAct(),vctr);
        pr *= p = mGtww.setIterProb(sd.setAwa(),d,sdP.getAwa(),rdD.getG(),vctr);
        if ( vctr<-1 && p==LogProb() ) cerr<<"\nERROR: no condition Gtww "<<d<<" "<<sdP.getAwa()<<" "<<rdD.getG()<<"\n\n";
      }
    }
    else {
      // 0,- _ (copy) case:
      if ( !mG_CPY.contains(sdP.getAct()) ) mG_CPY.setProb(sdP.getAct(),sdP.getAct() )=1.0;
      pr  = p = mG_CPY.setIterProb(sd.setAct(), sdP.getAct(), vctr);
      if ( !mG_CPY.contains(sdP.getAwa()) ) mG_CPY.setProb(sdP.getAwa(),sdP.getAwa())=1.0;
      pr *= p = mG_CPY.setIterProb(sd.setAwa(),sdP.getAwa(),vctr);
    }
    //cerr<<"    Sd "<<d<<" "<<rdD<<" "<<rd<<" "<<sdP<<" "<<sdU<<" : "<<sd<<" = "<<pr<<" ("<<vctr<<")\n";
    return pr;
  }
  friend pair<StringInput,SdModel*> operator>> ( StringInput si, SdModel& m ) { return pair<StringInput,SdModel*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,SdModel*> si_m, const char* psD ) {
    StringInput si;
    return ( (si=si_m.first>>"Ge "  >>si_m.second->mGe  >>psD)!=NULL ||
             (si=si_m.first>>"Gtaa ">>si_m.second->mGtaa>>psD)!=NULL ||
             (si=si_m.first>>"Gtaw ">>si_m.second->mGtaw>>psD)!=NULL ||
             (si=si_m.first>>"Gtww ">>si_m.second->mGtww>>psD)!=NULL ) ? si : StringInput(NULL);
  }
};
const HidVarCPT1DModel<G,LogProb> SdModel::mG_SUB(G_SUB);
HidVarCPT2DModel<G,G,LogProb> SdModel::mG_CPY;


//////////////////////////////////////// Joint models...

//////////////////// Reduce phase...

//// Model of R given S
class RModel : public SingleFactoredModel<RdModel> {
 public:
  LogProb setIterProb ( R::ArrayIterator<LogProb>& r, const S& sP, bool b1, int& vctr ) const {
    const RdModel& mRd = getM1();
    LogProb pr;
    pr  = mRd.setIterProb ( r.set(4-1), 4, Rd(sP.second) , sP.first.get(4-1), sP.first.get(3-1), b1, vctr );
    pr *= mRd.setIterProb ( r.set(3-1), 3, Rd(r.get(4-1)), sP.first.get(3-1), sP.first.get(2-1), b1, vctr );
    pr *= mRd.setIterProb ( r.set(2-1), 2, Rd(r.get(3-1)), sP.first.get(2-1), sP.first.get(1-1), b1, vctr );
    pr *= mRd.setIterProb ( r.set(1-1), 1, Rd(r.get(2-1)), sP.first.get(1-1), Sd_TOP           , b1, vctr );
    return pr;
  }
};


//////////////////// Shift phase...

//// Model of S given R and S
class SModel : public SingleFactoredModel<SdModel> {
 private:
  static const HidVarCPT1DModel<G,LogProb>   mG_SUB;
 public:
  LogProb setIterProb ( S::ArrayIterator<LogProb>& s, const R::ArrayIterator<LogProb>& r, const S& sP, int& vctr ) const {
    const SdModel& mSd = getM1();
    LogProb pr,p;
    pr  = mSd.setIterProb ( s.first.set(1-1), 1, Rd(r.get(2-1)), Rd(r.get(1-1)), sP.first.get(1-1), Sd_TOP              , vctr );
    pr *= mSd.setIterProb ( s.first.set(2-1), 2, Rd(r.get(3-1)), Rd(r.get(2-1)), sP.first.get(2-1), Sd(s.first.set(1-1)), vctr );
    pr *= mSd.setIterProb ( s.first.set(3-1), 3, Rd(r.get(4-1)), Rd(r.get(3-1)), sP.first.get(3-1), Sd(s.first.set(2-1)), vctr );
    pr *= mSd.setIterProb ( s.first.set(4-1), 4, Rd(sP.second) , Rd(r.get(4-1)), sP.first.get(4-1), Sd(s.first.set(3-1)), vctr );
    if ( G(s.first.set(4-1).second)!=G_SUB &&
         G(s.first.set(4-1).second).isTerm()!=B_1 ) {
      pr *= p = mSd.mGe.setIterProb (s.second, 5, G(s.first.set(4-1).second), vctr );
      if ( vctr<-1 && p==LogProb() ) cerr<<"\nERROR: no condition Ge 5 "<<G(s.first.set(4-1).second)<<"\n\n";
    } else {
      pr *= mG_SUB.setIterProb ( s.second, vctr );
    }
    ////cerr<<"  G "<<5<<" "<<G(sd4.second)<<" : "<<g<<" = "<<pr<<" ("<<vctr<<")\n";
    return pr;
  }
};
const HidVarCPT1DModel<G,LogProb> SModel::mG_SUB(G_SUB);


//////////////////// Overall...

//// Model of Y=R,S given S
class YModel : public DoubleFactoredModel<RModel,SModel> {
 public:
  typedef Y::ArrayIterator<LogProb> IterVal;
  S& setTrellDat ( S& s, const Y::ArrayIterator<LogProb>& y ) const {
    s.setVal(y.second);
    return s;
  }
  R setBackDat ( const Y::ArrayIterator<LogProb>& y ) const {
    R r;
    for(int i=0;i<4;i++)
      r.set(i)=Rd(y.first.get(i));
    return r;
  }
  LogProb setIterProb ( Y::ArrayIterator<LogProb>& y, const S& sP, const X& x, bool b1, int& vctr ) const {
    const RModel& mR = getM1();
    const SModel& mS = getM2();
    LogProb pr;
    pr  = mR.setIterProb ( y.first, sP, b1, vctr );
    if ( LogProb()==pr ) return pr;
    pr *= mS.setIterProb ( y.second, y.first, sP, vctr );
    return pr;
  }
  void update ( ) const { }
};
