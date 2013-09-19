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

#include "nl-linsep.h"
/*
class ConvolvedComplexMelVec {
 private:
  ComplexMelVec vMain;
  double        dConvo;
 public:
  void set ( const ComplexMelVec& v ) { vMain=v; for(int i=0;i<ComplexMelVec::SIZE;i++)dConvo+=vMain.get(i)*vMain.get(i); }
  static const int SIZE = ComplexMelVec::SIZE+1;
  double& set ( int i )       { return (i<ComplexMelVec::SIZE)?vMain.set(i):dConvo; }
  double  get ( int i ) const { return (i<ComplexMelVec::SIZE)?vMain.get(i):dConvo; }
  double& operator[] ( int i )       { return set(i); }
  double  operator[] ( int i ) const { return get(i); }
  double        operator*  ( const ConvolvedComplexMelVec& v ) const { double d; for(int i=0;i<SIZE;i++)d+=get(i)*v.get(i); return d; }  // inner product
  friend pair<StringInput,ConvolvedComplexMelVec*> operator>> ( StringInput str, ConvolvedComplexMelVec& v ) {
    return pair<StringInput,ConvolvedComplexMelVec*>(str,&v); }
  friend StringInput operator>> ( pair<StringInput,ConvolvedComplexMelVec*> delimbuff, const char* ps ) {
    StringInput str = delimbuff.first>>delimbuff.second->vMain>>",">>delimbuff.second->dConvo>>ps;
    for(int i=0;i<ComplexMelVec::SIZE;i++)delimbuff.second->dConvo+=delimbuff.second->vMain.get(i)*delimbuff.second->vMain.get(i);
    return str; }
  friend ostream& operator<< ( ostream& os, const ConvolvedComplexMelVec& v ) { return os<<v.vMain<<","<<v.dConvo; }
};

class TrainingExample : public DelimitedJoint2DRV<psX,Psymb,psBar,ConvolvedComplexMelVec,psX> { };
*/


////////////////////////////////////////////////////////////////////////////////

class binuint {
 private:
  uint b;
 public:
  // Constructor / destructor methods...
  binuint ( )        : b(0) { }
  binuint ( uint i ) : b(i) { }
  // Specification methods...
  binuint& operator+= ( binuint i ) { b+=i.b; return *this; }
  binuint& operator-= ( binuint i ) { b-=i.b; return *this; }
  binuint& operator*= ( binuint i ) { b*=i.b; return *this; }
  binuint& operator/= ( binuint i ) { b/=i.b; return *this; }
  binuint& operator%= ( binuint i ) { b%=i.b; return *this; }
  binuint& operator++ ( )           { b++; return *this; }
  // Extractor methods...
  binuint operator+ ( binuint i ) const { return (b+i.b); }
  binuint operator- ( binuint i ) const { return (b-i.b); }
  binuint operator* ( binuint i ) const { return (b*i.b); }
  binuint operator/ ( binuint i ) const { return (b/i.b); }
  binuint operator% ( binuint i ) const { return (b%i.b); }
  bool operator== ( binuint i ) const { return (b==i.b); }
  bool operator!= ( binuint i ) const { return (b!=i.b); }
  bool operator<  ( binuint i ) const { return (b<i.b); }
  bool operator>  ( binuint i ) const { return (b>i.b); }
  bool operator<= ( binuint i ) const { return (b<=i.b); }
  bool operator>= ( binuint i ) const { return (b>=i.b); }
  size_t getHashKey ( ) const { return b; }
  // Input / output methods...
  friend StringInput operator>> ( StringInput si, binuint& i ) {
    if(si==NULL) return si;
    i.b=0; 
    for ( char c=si[0]; '0'<=c && c<='1'; ++si,c=si[0])
      { i.b=i.b*2+c-'0'; }
    return si; }
  friend ostream& operator<< ( ostream& os, binuint i ) { for(int e=uint(log2(i.b));e>=0;e--)os <<((i.b>>e)%2); return os;  }  
  friend String&  operator<< ( String& str, binuint i ) { for(int e=uint(log2(i.b));e>=0;e--)str<<((i.b>>e)%2); return str; }  
};

////////////////////////////////////////////////////////////////////////////////

template<class YF,class M>
class ObliqueDTreeModel {

 private:

  ////=M////typedef SimpleLinSepModel<Q,X> SepModel;
  //typedef int TreeAddr;
  map<binuint,M>                    hag;
  map<binuint,typename M::ProbType> hap;

 public:

  typedef typename M::InputTrainingExample InputTrainingExample;

  void train ( SubArray<SafePtr<const InputTrainingExample> >, const YF&, binuint=1 );

  typename M::ProbType getProb ( binuint y, const typename M::CondVarType& ) const;

  friend pair<StringInput,ObliqueDTreeModel<YF,M>*> operator>> ( StringInput si, ObliqueDTreeModel<YF,M>& m ) {
    return pair<StringInput,ObliqueDTreeModel<YF,M>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,ObliqueDTreeModel<YF,M>*> si_m, const char* psD ) {
    if (si_m.first==NULL) return si_m.first;
    binuint branch=0;
    StringInput si=si_m.first>>branch>>" ";
    //cerr<<"?^?^? "<<si<<"\n";
    if ( si==NULL ) return si_m.first;
    //fprintf(stderr," .. %x\n",delimbuff.second);
    if ( '='==si[0] ) return si>>"= ">>si_m.second->hag[branch]>>psD;
    else              return si>>": 1 = ">>si_m.second->hap[branch]>>psD;
  }

  friend ostream& operator<< ( ostream& os, const ObliqueDTreeModel<YF,M>& m ) {
    return os<<pair<const String*,const ObliqueDTreeModel<YF,M>*>(&String(""),&m); }
  friend pair< const String*, const ObliqueDTreeModel<YF,M>*> operator* ( const String& s, const ObliqueDTreeModel<YF,M>& m ) {
    return pair<const String*,const ObliqueDTreeModel<YF,M>*>(&s,&m); }
  friend ostream& operator<< ( ostream& os, const pair<const String*,const ObliqueDTreeModel<YF,M>*> s_m ) {
    // For each tree branch...
    binuint b=1; do {
      // If non-terminal branch, write gradient...
      if ( s_m.second->hag.find(b)!=s_m.second->hag.end() && s_m.second->hap.find(b)==s_m.second->hap.end() )
        os<<*s_m.first<<b<<" = "    <<s_m.second->hag.find(b)->second<<"\n";
      // If terminal branch, write distrib...
      else if ( s_m.second->hap.find(b)!=s_m.second->hap.end() )
        os<<*s_m.first<<b<<" : 1 = "<<s_m.second->hap.find(b)->second<<"\n";

      // If non-terminal branch, recurse...
      if ( s_m.second->hag.find(b)!=s_m.second->hag.end() ) b*=2;
      // If terminal branch, pop odds then advance...
      else { while(b%2==1)b/=2; ++b; }
    } while ( b!=1 );
    //os<<"done.\n";
    return os;
  }
};

////////////////////////////////////////
template<class YF,class M>
  void ObliqueDTreeModel<YF,M>::train ( SubArray<SafePtr<const InputTrainingExample> > ape, const YF& yfn, binuint branch ) {

  cerr<<"  starting oblidtree branch "<<branch<<": "<<ape.size()<<" examples...\n";

  //// find separator

  double iP=0.0,iN=0.0;
  for ( unsigned int n=0; n<ape.size(); n++ ) { if (yfn(ape[n].getRef().first)) iP++; else iN++; }
  //cerr<<"  iN="<<iN<<" iP="<<iP<<" in this subtree\n";

  //if ( iP>10 && iN>10 )
  hag[branch].train(ape,yfn,iP,iN);

  //// divide data

  unsigned int iSplitPt = ape.size();
  // For each example...
  for ( unsigned int i=0; i<iSplitPt; i++ ) {
    // Determine side of gradient...
    bool yEst = hag[branch].classify(ape[i].getRef().second);
    ////double yEst=0.0;
    ////for ( int j=0; j<O::SIZE; j++ ) yEst += hag[branch][j]*ape[i].getRef().second[j];
    // Swap greater-than-one examples to end of array...
    if ( yEst ) {  ////( yEst >= 1.0 ) {
      iSplitPt--;
      SafePtr<const InputTrainingExample> peTemp = ape[i];
      ape[i] = ape[iSplitPt];
      ape[iSplitPt] = peTemp;
      i--;
    }
  }
  cerr<<"  oblidtree branch "<<branch<<": split 0 < "<<iSplitPt<<" < "<<ape.size()<<"  parent%pos="<<(iP/ape.size());
  if ( iSplitPt>0 && iSplitPt<ape.size() ) {
    double pPos=0.0;
    for ( unsigned int n=0; n<iSplitPt; n++ ) if(yfn(ape[n].getRef().first))pPos++;
    cerr<<",left%pos=("<<pPos<<"/"<<iSplitPt;
    pPos/=iSplitPt;
    cerr<<")="<<pPos;
    pPos=0.0;
    for ( unsigned int n=iSplitPt; n<ape.size(); n++ ) if(yfn(ape[n].getRef().first))pPos++;
    cerr<<",right%pos=("<<pPos<<"/"<<(ape.size()-iSplitPt);
    pPos/=(ape.size()-iSplitPt);
    cerr<<")="<<pPos;
  }
  cerr<<"\n";
  // Recurse on divided array: greater-than-ones at right...
  if ( iP > 0 && iN > 0 && iSplitPt > 100 && ape.size()-iSplitPt > 100 ) {
    train ( SubArray<SafePtr<const InputTrainingExample> >(ape,0,iSplitPt), yfn, branch*2   );
    train ( SubArray<SafePtr<const InputTrainingExample> >(ape,iSplitPt),   yfn, branch*2+1 );
  }
  else {
    // Add prob...
    hap[branch]=(double(iP)+.1)/(double(ape.size())+.1);
    ////cout<<"terminal prob of "<<qTarget<<" at "<<branch<<" = "<<iP<<"/"<<ape.size()<<" = "<<hap[branch]<<"\n";
  }
}

////////////////////////////////////////
template<class YF,class M>
typename M::ProbType ObliqueDTreeModel<YF,M>::getProb ( binuint y, const typename M::CondVarType& o ) const {

  //const tr1::unordered_map<binuint,P>& hap = hhap.get(q);
  //const double SLOPE = 20;
  //// For each tree branch...
  //Prob prWidth=1.0; Prob prQ=0.0; int b=1; Prob prL=0.0; Prob prR=0.0; map<binuint,double> widthcache; widthcache[b]=prWidth;
  //do {
  //  // If nonterminal branch, push left...
  //  if ( hag.find(b)!=hag.end() ) {
  //    // Calc L/R probs given dist of o from separator g...
  //    prR=1.0/(1.0+exp(-(o*hag.find(b)->second-1.0)*SLOPE)); prL=1.0-prR; b*=2; widthcache[b]=prWidth*=prL;
  //  }
  //  // If terminal branch, pop all rights, then advance left to right...
  //  else if ( hap.find(b)!=hap.end() ) {
  //    prQ += prWidth * hap.find(b)->second.toDouble();
  //    while(b%2==1){ b/=2; }
  //    prWidth=(b<=1)?0.0:widthcache[b/2]; b++; prR=(b<=1)?0.0:1.0/(1.0+exp(-(o*hag.find(b/2)->second-1.0)*SLOPE)); widthcache[b]=prWidth*=prR;
  //  }
  //} while ( b!=1 );
  //return P(prQ);

  ////cout<<" :: "<<q<<"  "<<(X)o<<"\n";
  binuint b=1; while ( hag.find(b)!=hag.end() ) {
    ////cout<<"    "<<b<<"    "<<hag.find(b)->second<<"\n";
    //b=(o * hag.find(b)->second < 1.0) ? b*2 : b*2+1;
    b = (hag.find(b)->second.classify(o)==false) ? b*2 : b*2+1;
  }
  //cout<<"  SepTreeObsModel: "<<q<<" "<<b<<" "<<hhap.get(q).find(b)->second<<"\n";
  return (hap.find(b)==hap.end()) ? typename M::ProbType() : (y==0) ? typename M::ProbType(1.0-hap.find(b)->second.toDouble()) : hap.find(b)->second;
}

