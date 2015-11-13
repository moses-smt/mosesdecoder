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

#ifndef _NL_MIXTURE__
#define _NL_MIXTURE__

#include "nl-randvar.h"
#include "nl-list.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Mixture Model
//
////////////////////////////////////////////////////////////////////////////////

template <template <class MY> class M,class Y,class C>
class Mixture2DModel : public Generic1DModel<Y,Prob> {
 private:
  // Static data members...
  static bool bModeled;
  // Data members...
  CPT1DModel<C,Prob>  mC;
  SimpleHash<C,M<Y> > mY_giv_C;
 public:
  Mixture2DModel ( )                 : mC()  { }
  Mixture2DModel ( const string& s ) : mC(s) { }
  // Specification methods...
  CPT1DModel<C,Prob>& setMixingModel ( ) { return mC; }
  Prob& setComponentProb  ( const C& c ) { return mC.setProb(c);       }
  M<Y>& setComponentModel ( const C& c ) { return mY_giv_C.set(c); }
  // Extraction methods...
  Prob        getComponentProb  ( const C& c ) { return mC.getProb(c);       }
  const M<Y>& getComponentModel ( const C& c ) { return mY_giv_C.get(c); }
  Prob getProb ( const Y& y ) const { Prob pr=0.0;
                                      C c; for ( bool b=c.setFirst(); b; b=c.setNext() )
                                             pr += ( mC.getProb(c) * mY_giv_C.get(c).getProb(y) );
                                      return pr; }
  // Input / output methods...
  bool readFields  ( char* as[], int numF )               { return ( mC.readFields(as,numF) || mY_giv_C.set(C(as[1])).readFields(as+1,numF-1) ); }
  void writeFields ( FILE* pf, string sPref )       const { mC.writeFields(pf,sPref);   C c; for ( bool b=c.setFirst(); b; b=c.setNext() )
                                                                                               mY_giv_C.get(c).writeFields(pf,sPref+" "+c.getString()); }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <template <class MY> class M,class Y,class X,class C>
class Mixture3DModel : public Generic2DModel<Y,X,Prob> {
 private:
  // Static data members...
  static bool bModeled;
  // Data members...
  CPT2DModel<C,X,Prob>             mC_giv_X;
  SimpleHash<Joint2DRV<X,C>,M<Y> > mY_giv_X_C;
 public:
  Mixture3DModel ( )                 : mC_giv_X()  { }
  Mixture3DModel ( const string& s ) : mC_giv_X(s) { }
  // Specification methods...
  Prob& setComponentProb  ( const X& x, const C& c ) { return mC_giv_X.setProb(c,x); }
  M<Y>& setComponentModel ( const X& x, const C& c ) { return mY_giv_X_C.set(Joint2DRV<X,C>(x,c)); }
  // Extraction methods...
  Prob getProb ( const Y& y, const X& x ) const { Prob pr=0.0;
                                                  C c; for ( bool b=c.setFirst(); b; b=c.setNext() )
                                                         pr += ( mC_giv_X.getProb(c,x) * mY_giv_X_C.get(Joint2DRV<X,C>(x,c)).getProb(y) );
                                                  return pr; }
  // Input / output methods...
  bool readFields  ( char* as[], int numF )     { return ( mC_giv_X.readFields(as,numF) || mY_giv_X_C.set(Joint2DRV<X,C>(X(as[1]),C(as[2]))).readFields(as+2,numF-2) ); }
  void writeFields ( FILE* pf, string sPref ) const { mC_giv_X.writeFields(pf,sPref);
                                                            X x; for ( bool bx=x.setFirst(); bx; bx=x.setNext() ) {
                                                                   C c; for ( bool b=c.setFirst(); b; b=c.setNext() ) {
                                                                          mY_giv_X_C.get(Joint2DRV<X,C>(x,c)).writeFields(pf,sPref+" "+c.getString()); } } }
};

////////////////////////////////////////////////////////////////////////////////
//
//  Trainable Mixture Model
//
////////////////////////////////////////////////////////////////////////////////

template <template <class MY> class M,class Y,class C>
class TrainableMixture2DModel : public Mixture2DModel<M,Y,C> {
// private:
//  LogPDFVal logpdfPrevDataAvg;
 public:
//  // Constructor / destructor methods...
//  TrainableMixture2DModel ( ) : logpdfPrevDataAvg(log(0.0)) { }
  // Specification methods...
  void updateFields ( const List<Joint2DRV<Y,Prob> >&, const PDFVal, bool& );
  void train        ( List<Joint2DRV<Y,Prob> >&, const int, const PDFVal );
};

////////////////////////////////////////
template <template <class MY> class M,class Y,class C>
void TrainableMixture2DModel<M,Y,C>::updateFields ( const List<Joint2DRV<Y,Prob> >& lyp, const PDFVal WEIGHT_LIMIT, bool& bShouldStop ) {
  LogPDFVal logpdfData = 0.0;
  CPT1DModel<C,Prob>                             mprPseudoEmpC;        // pseudo-empirical prob marginal
  SimpleHash<C,List<pair<const Y*,Prob> > >      alypPseudoEmpY_giv_C; // pseudo-empirical prob conditional

  //// E step: create pseudo-annotated (expectation) corpus by re-distributing empirical probabilities over C, according to model parameters...

  C c;
  for ( bool b=c.setFirst(); b; b=c.setNext() )
    setComponentModel(c).precomputeVarianceTerms();

  // For each Y...
  for ( const ListedObject<Joint2DRV<Y,Prob> >* pyp=lyp.getFirst(); pyp; pyp=lyp.getNext(pyp) ) {
    const Y&    y      = pyp->getSub1();  // data value
    const Prob& prEmpY = pyp->getSub2();  // empirical prob

    CPT1DModel<C,PDFVal> mpdfEstCY;      // model-estimated pdf
    PDFVal               pdfEstY = 0.0;  // model-estimated pdf marginal

    // For each C, compute model-estimated pdfs...
    C c; for ( bool b=c.setFirst(); b; b=c.setNext() ) {
      PDFVal pdfEstCY       = getComponentProb(c) * getComponentModel(c).getProb(y);
//      fprintf(stderr," ???%g %d %g\n",(double)pdfEstCY,(int)c,getComponentModel(c).getProb(y)); ////y.write(stderr); fprintf(stderr,"\n");
      mpdfEstCY.setProb(c)  = pdfEstCY;
      pdfEstY              += pdfEstCY;      // marginalize out C     (marginal estimated pdf of each y)
    }

    // Update total...
    logpdfData += log(pdfEstY); // product of each datum (joint estimated pdf of training corpus)
    //fprintf(stderr,"..%g %g\n",pdfEstY,logpdfData);

    // For each C, compute psuedo-empirical probs (model-estimated * empirical)...
    for ( bool b=c.setFirst(); b; b=c.setNext() ) {
      Prob prEstC_giv_Y                            = mpdfEstCY.getProb(c) / pdfEstY;   // so these are normalized distribs over C
      Prob prPseudoEmpCY                           = prEmpY * prEstC_giv_Y;
      alypPseudoEmpY_giv_C.set(c).add()            = pair<const Y*,Prob> ( &pyp->getSub1(), prPseudoEmpCY );
      mprPseudoEmpC.setProb(c)                    += prPseudoEmpCY;      // marginalize out Y
    }
  }
  // Renormalize sub-lists...
  for ( bool b=c.setFirst(); b; b=c.setNext() )
    for ( ListedObject<pair<const Y*,Prob> >* pyp=alypPseudoEmpY_giv_C.set(c).setFirst(); pyp; pyp=alypPseudoEmpY_giv_C.set(c).setNext(pyp) )
      pyp->second /= mprPseudoEmpC.getProb(c);

  //// M step: update model parameters using MLE (relative frequency estimation) on pseudo-annotated corpus...

  for ( bool b=c.setFirst(); b; b=c.setNext() ) {
    // If any change exceeds thresh, continue...
    if ( bShouldStop && ( getComponentProb(c)-mprPseudoEmpC.getProb(c) >  WEIGHT_LIMIT ||
                          getComponentProb(c)-mprPseudoEmpC.getProb(c) < -WEIGHT_LIMIT ) ) bShouldStop = false;
    // Update probs...
    setComponentProb(c)            = mprPseudoEmpC.getProb(c);
    setComponentModel(c).setFields ( alypPseudoEmpY_giv_C.get(c) );
    //////getComponentModel(c).writeFields(stderr,c.getString());
  }

  //////Mixture2DModel<M,Y,C>::writeFields(stderr,"");
  fprintf(stderr,"  log pdf data: %g\n",(double)logpdfData);
//  // Quit if total prob doesn't improve enough...
//  fprintf(stderr,"  log avg pdf: %g - %g < %g ORLY? %d\n",
//          (double)logpdfDataAvg,(double)logpdfPrevDataAvg,(double)WEIGHT_LIMIT,
//          ((logpdfDataAvg - logpdfPrevDataAvg) < WEIGHT_LIMIT)  );
//  bShouldStop = ( (logpdfDataAvg - logpdfPrevDataAvg) < WEIGHT_LIMIT );
//  logpdfPrevDataAvg = logpdfDataAvg;
}

////////////////////////////////////////
template <template <class MY> class M,class Y,class C>
void TrainableMixture2DModel<M,Y,C>::train ( List<Joint2DRV<Y,Prob> >& lyp, const int EPOCH_LIMIT, const PDFVal WEIGHT_LIMIT ) {

  // Normalize model...
  Mixture2DModel<M,Y,C>::setMixingModel().normalize();  // OUGHT NOT TO BE NECESSARY!!!

  // Normalize input...
  PDFVal pdfNorm = 0.0;
  for ( const ListedObject<Joint2DRV<Y,Prob> >* pyp=lyp.getFirst(); pyp; pyp=lyp.getNext(pyp) ) pdfNorm += pyp->getSub2();
  for ( ListedObject<Joint2DRV<Y,Prob> >*       pyp=lyp.setFirst(); pyp; pyp=lyp.setNext(pyp) ) pyp->setSub2() /= pdfNorm;

  // Iterate over each epoch...
  bool bShouldStop = true;
  for ( int epoch=0; epoch<EPOCH_LIMIT; epoch++ ) {
    fprintf(stderr,"---------- EPOCH %d/%d ----------\n",epoch,EPOCH_LIMIT);
    clock_t start = clock();
    updateFields ( lyp, WEIGHT_LIMIT, bShouldStop ) ;
    clock_t end = clock();
    fprintf ( stderr, "epoch time: %d ticks or %f seconds\n", end-start, ((double)end - start)/CLOCKS_PER_SEC ) ;
    if (bShouldStop) break;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <template <class MY> class M,class Y,class X,class C>
class TrainableMixture3DModel : public Generic2DModel<Y,X,C> {
 private:
  string sId;
  SimpleHash<X,List<Joint2DRV<Y,Prob> > >       alyp;
  SimpleHash<X,TrainableMixture2DModel<M,Y,C> > am;
 public:
  // Constructor / destructor methods...
  TrainableMixture3DModel ( string s ) : sId(s) { }
  // Specification methods...
  void train ( const int, const PDFVal );
  void train ( const List<Joint3DRV<X,Y,Prob> >&, const int, const PDFVal );
  // Extraction methods...
  Prob getProb ( const Y& y, const X& x ) const { return am.get(x).getProb(y); }
  // Input / output methods...
  bool readData    ( char* as[], int );
  bool readFields  ( char* as[], int numF ) { return ( am.set(X(as[1])).readFields(as+1,numF-1) ); }
  void writeFields ( FILE*, string sPref );
};

////////////////////////////////////////
template <template <class MY> class M,class Y,class X,class C>
void TrainableMixture3DModel<M,Y,X,C>::train ( const int EPOCH_LIMIT, const PDFVal WEIGHT_LIMIT ) {
  // Update each subphone from list...
  int ctr = 0;
  X x; for ( bool b=x.setFirst(); b; b=x.setNext() ) {
    if (OUTPUT_NOISY) fprintf(stderr,"==================== SUBMODEL %s (number %d) %d data pts ====================\n",
                              x.getString().c_str(),ctr++,alyp.get(x).getCard());
    am.set(x).train ( alyp.set(x), EPOCH_LIMIT, WEIGHT_LIMIT );
  }
}

////////////////////////////////////////
template <template <class MY> class M,class Y,class X,class C>
void TrainableMixture3DModel<M,Y,X,C>::train ( const List<Joint3DRV<X,Y,Prob> >& lxyp, const int EPOCH_LIMIT, const PDFVal WEIGHT_LIMIT ) {
  // Chop list into phone-specific sub-lists...
  ListedObject<Joint3DRV<X,Y,Prob> >* pxyp;
  for ( pxyp=lxyp.getFirst(); pxyp; pxyp=lxyp.getNext(pxyp) )
    alyp.set(pxyp->getSub1()).add() = Joint2DRV<Y,Prob> ( pxyp->getSub2(), pxyp->getSub3() );
  // Update each subphone from list...
  train(EPOCH_LIMIT,WEIGHT_LIMIT);
}

////////////////////////////////////////
template <template <class MY> class M,class Y,class X,class C>
bool TrainableMixture3DModel<M,Y,X,C>::readData ( char* as[], int numFields ) {
  if ( /*as[0]!=sId+"dat" ||*/ numFields!=3 ) return false;
  alyp.set(X(as[1])).add() = Joint2DRV<Y,Prob>(Y(as[2]),Prob(1.0));
  return true;
}

////////////////////////////////////////
template <template <class MY> class M,class Y,class X,class C>
void TrainableMixture3DModel<M,Y,X,C>::writeFields  ( FILE* pf, string sPref ) {
  X x; for ( bool b=x.setFirst(); b; b=x.setNext() ) {
    am.get(x).writeFields(pf,sPref+" "+x.getString());
  }
}


#endif /*_NL_MIXTURE__*/

