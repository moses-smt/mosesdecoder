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

#ifndef _NL_GAUSS__
#define _NL_GAUSS__

#include <vector>
#include <string>
#include <math.h>
#include "nl-cpt.h"
#include "nl-prob.h"
#include "nl-list.h"

using namespace std;

static const PDFVal MEAN_THRESHOLD = 0.01; //0.0001; //0.001
static const PDFVal VARIANCE_THRESHOLD = 0.01; //0.0001; //0


////////////////////////////////////////////////////////////////////////////////
//
//  Diagonal Multivariate Gaussian Model
//
////////////////////////////////////////////////////////////////////////////////

template <class Y>
class DiagGauss1DModel : public Generic1DModel<Y,PDFVal> {
 private:
  // Member variables...
  string                     sId;
  bool                       bModeled;
  PDFVal                     prInvPowSqrt2PI;
  SimpleHash<Id<int>,PDFVal> aMeans;
  SimpleHash<Id<int>,PDFVal> aVariances;
  PDFVal                     prInvRootNormVariances;
  PDFVal                     prProduct;
  SimpleHash<Id<int>,PDFVal> algprNegHalfInvVariances;
 public:
  // Constructor / destructor methods...
  DiagGauss1DModel ( )                 :         bModeled(false) { }
  DiagGauss1DModel ( const string& s ) : sId(s), bModeled(false) { }
  // Specification methods...
  void precomputeVarianceTerms ( ) ;
  PDFVal& setMean           ( int i ) { return aMeans.set(i);                   }
  PDFVal& setVariance       ( int i ) { return aVariances.set(i);               }
  PDFVal& setInvRootNormVar ( )       { return prInvRootNormVariances;          }
  PDFVal& setNegHalfInvVar  ( int i ) { return algprNegHalfInvVariances.set(i); }
  // Extraction methods...
  PDFVal  getMean           ( int i )    const { return aMeans.get(i);                   }
  PDFVal  getVariance       ( int i )    const { return aVariances.get(i);               }
  PDFVal  getInvRootNormVar ( )          const { return prInvRootNormVariances;          }
  PDFVal  getNegHalfInvVar  ( int i )    const { return algprNegHalfInvVariances.get(i); }
  int     getNumFeats       ( )          const { return Y::getSize(); }
  PDFVal  getProb           ( const Y& ) const ;
  // Input / output methods...
  bool    readFields  ( char*[], int ) ;
  void    writeFields ( FILE*, const string& ) const ;
};

////////////////////////////////////////
template <class Y>
inline void DiagGauss1DModel<Y>::precomputeVarianceTerms ( ) {
  // Inverse square root of norm of variances...
  setInvRootNormVar() = 1.0;
  for ( int i=0; i<getNumFeats(); i++ )  setInvRootNormVar() *= 1.0/sqrt(getVariance(i));
  // Negative half of inverse of variances...
  for ( int i=0; i<getNumFeats(); i++ )  setNegHalfInvVar(i) = -1.0/(2.0*getVariance(i));
  // Derived from variance terms...
  prInvPowSqrt2PI = 1.0/pow(sqrt(2.0*M_PI),getNumFeats());
  prProduct       = prInvPowSqrt2PI * getInvRootNormVar();
  bModeled = true;
}

////////////////////////////////////////
template <class Y>
inline PDFVal DiagGauss1DModel<Y>::getProb ( const Y& y ) const {
//  fprintf(stderr,"--------------------\n");
//  y.write(stderr);
//  fprintf(stderr,"\n----------\n");
//  writeFields(stderr,"");
  assert(bModeled);
  PDFVal logprob = 0.0;
  for ( int i=0; i<getNumFeats(); i++ )
    logprob += getNegHalfInvVar(i) * pow(y.get(i)-getMean(i),2.0);
//  for ( int i=0; i<getNumFeats(); i++ )
//    fprintf(stderr,"%d %g\n", i, getNegHalfInvVar(i) * pow(y.get(i)-getMean(i),2.0));
//  fprintf(stderr,"----------> %g\n",prProduct * exp(logprob));
  return ( prProduct * exp(logprob) ) ;
}

////////////////////////////////////////
template <class Y>
bool DiagGauss1DModel<Y>::readFields ( char* as[], int numFields ) {
  if ( 0==strcmp(as[1],"m") && numFields>2 ) {
    char* psT;
    for(int i=0;i<getNumFeats();i++)
      setMean(i)=atof(strtok_r((0==i)?as[2]:NULL,"_",&psT));
  }
  else if ( 0==strcmp(as[1],"v") && numFields>2 ) {
    char* psT;
    for(int i=0;i<getNumFeats();i++)
      setVariance(i)=atof(strtok_r((0==i)?as[2]:NULL,"_",&psT));
  }
  else return false;
  return true;
}

////////////////////////////////////////
template <class Y>
void DiagGauss1DModel<Y>::writeFields ( FILE* pf, const string& sPref ) const {
  fprintf(pf,"%s m = ",sPref.c_str());
  for(int i=0; i<getNumFeats(); i++) fprintf(pf,"%s%f",(0==i)?"":"_",getMean(i));
  fprintf ( pf, "\n" ) ;

  fprintf(pf,"%s v = ",sPref.c_str());
  for(int i=0; i<getNumFeats(); i++) fprintf(pf,"%s%f",(0==i)?"":"_",getVariance(i));
  fprintf ( pf, "\n" ) ;
}


////////////////////////////////////////////////////////////////////////////////

/*
template <class Y,class X>
class DiagGauss2DModel : public Generic2DModel<Y,X,PDFVal> {
 private:
  // Member variables...
  string                             sId;
  SimpleHash<X,DiagGauss1DModel<Y> > mMY_giv_X;
 public:
  // Constructor / destructor methods...
  DiagGauss2DModel ( const string& s ) : sId(s) { }
  // Extraction methods...
  Prob getProb ( const Y& y, const X& x ) const { return mMY_giv_X.get(x).getProb(y); }
  // Input / output methods...
  bool readFields  ( char* as[], int numFields ) {
    ////if ( as[0]!=sId ) return false; // HAVE TO CHECK IN CALLIN FN NOW
    if      ( 0==strcmp(as[1],"m") && numFields>3 )
      for ( int i=0; i<numFields-3; i++ ) mMY_giv_X.set(X(as[2])).setMean(i)     = atof(as[i+4]) ;
    else if ( 0==strcmp(as[1],"v") && numFields>3 )
      for ( int i=0; i<numFields-3; i++ ) mMY_giv_X.set(X(as[2])).setVariance(i) = atof(as[i+4]) ;
    else return false;
    return true;
  }
  void writeFields ( FILE* pf, const string& sPref ) const {
    X x;
    for(bool b=x.setFirst(); b; b=x.setNext())
      { fprintf(pf,"%s m ",sPref.c_str()); x.write(pf); fprintf(pf," =");
        for(int i=0; i<Y::getSize(); i++) fprintf(pf," %f",mMY_giv_X.getProb(x).getMean(i));
        fprintf ( pf, "\n" ) ; }
    for(bool b=x.setFirst(); b; b=x.setNext())
      { fprintf(pf,"%s v ",sPref.c_str()); x.write(pf); fprintf(pf," =");
        for(int i=0; i<Y::getSize(); i++) fprintf(pf," %f",mMY_giv_X.getProb(x).getVariance(i));
        fprintf ( pf, "\n" ) ; }
  }
};

////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2>
class DiagGauss3DModel : public Generic3DModel<Y,X1,X2,PDFVal> {
 private:
  // Member variables...
  string                                            sId;
  SimpleHash<Joint2DRV<X1,X2>,DiagGauss1DModel<Y> > mMY_giv_X1_X2;
 public:
  // Constructor / destructor methods...
  DiagGauss3DModel ( const string& s ) : sId(s) { }
  // Extraction methods...
  Prob getProb ( const Y& y, const X1& x1, const X2& x2 ) const { return mMY_giv_X1_X2.get(x1,x2).getProb(y); }
  // Input / output methods...
  bool readFields  ( char* as[], int numFields ) {
    if ( as[0]!=sId ) return false;
    if      ( 0==strcmp(as[1],"m") && numFields>4 )
      for ( int i=0; i<numFields-4; i++ ) mMY_giv_X1_X2.set(Joint2DRV<X1,X2>(X1(as[2]),X2(as[2]))).setMean(i)     = atof(as[i+4]) ;
    else if ( 0==strcmp(as[1],"v") && numFields>4 )
      for ( int i=0; i<numFields-4; i++ ) mMY_giv_X1_X2.set(Joint2DRV<X1,X2>(X1(as[2]),X2(as[2]))).setVariance(i) = atof(as[i+4]) ;
    else return false;
    return true;
  }
  void writeFields ( FILE* pf, string& sPref ) const {
    X1 x1; X2 x2;
    for(bool b1=x1.setFirst(); b1; b1=x1.setNext()) {
      for(bool b2=x2.setFirst(); b2; b2=x2.setNext())
        { fprintf(pf,"%s m ",sPref.c_str()); x1.write(pf); fprintf(pf," "); x2.write(pf); fprintf(pf," =");
          for(int i=0; i<Y::getSize(); i++) fprintf(pf," %f",mMY_giv_X1_X2.get(Joint2DRV<X1,X2>(x1,x2)).getMean(i));
          fprintf(pf,"\n"); }
      for(bool b2=x2.setFirst(); b2; b2=x2.setNext())
        { fprintf(pf,"%s v ",sPref.c_str()); x1.write(pf); fprintf(pf," "); x2.write(pf); fprintf(pf," =");
          for(int i=0; i<Y::getSize(); i++) fprintf(pf," %f",mMY_giv_X1_X2.get(Joint2DRV<X1,X2>(x1,x2)).getVariance(i));
          fprintf(pf,"\n"); }
    }
  }
};
*/

////////////////////////////////////////////////////////////////////////////////
//
//  Trainable Diagonal Multivariate Gaussian Model
//
////////////////////////////////////////////////////////////////////////////////

template <class Y>
class TrainableDiagGauss1DModel : public DiagGauss1DModel<Y> {
 public:
  TrainableDiagGauss1DModel ( )                 : DiagGauss1DModel<Y>() { }
  TrainableDiagGauss1DModel ( const string& s ) : DiagGauss1DModel<Y>(s) { }
  // input / output methods...
  void setFields ( const List<pair<const Y*,Prob> >& ) ;
};

////////////////////////////////////////
template <class Y>
void TrainableDiagGauss1DModel<Y>::setFields ( const List<pair<const Y*,Prob> >& lyp ) {

  // For each dimension...
  for ( int i=0; i<DiagGauss1DModel<Y>::getNumFeats(); i++ ) {

    // Calc means...
    double curMean = DiagGauss1DModel<Y>::getMean(i);
    DiagGauss1DModel<Y>::setMean(i) = 0.0;

    // For each Y...
    for ( const ListedObject<pair<const Y*,Prob> >* pyp=lyp.getFirst(); pyp; pyp=lyp.getNext(pyp) ) {
      const Y&    y      = *pyp->first;  // data value
      const Prob& prEmpY = pyp->second;  // empirical prob

      //printf("cal mean i=%d x1=%s x2=%s aaaprYpsi.get(yd,x1,x2)=%f\n", i, x1.getString(), x2.getString(), (double)aaaprYpsi.getProb(yd,x1,x2));
      DiagGauss1DModel<Y>::setMean(i) += prEmpY * y.get(i);
    }

//    // If any change exceeds thresh, continue...
//    if ( bShouldStop && ( curMean - DiagGauss1DModel<Y>::getMean(i) >  MEAN_THRESHOLD ||
//                          curMean - DiagGauss1DModel<Y>::getMean(i) < -MEAN_THRESHOLD ) ) bShouldStop = false;

    //printf("cal mean i=%d getMean(i)=%f\n", i, DiagGauss1DModel<Y>::getMean(i));

    // Calc variances...
    double curVar = DiagGauss1DModel<Y>::getVariance(i);
    DiagGauss1DModel<Y>::setVariance(i) = 0.0;

    // For each Y...
    for ( const ListedObject<pair<const Y*,Prob> >* pyp=lyp.getFirst(); pyp; pyp=lyp.getNext(pyp) ) {
      const Y&    y      = *pyp->first;  // data value
      const Prob& prEmpY = pyp->second;  // empirical prob

      //printf("cal var i=%d yd=%s %f %f %f\n", i, yd.getString(), aaaprYpsi.get(yd,x1,x2), getMean(x1,x2,i), yd.get(i));
      DiagGauss1DModel<Y>::setVariance(i) += prEmpY * pow(DiagGauss1DModel<Y>::getMean(i)-y.get(i),2) ;
    }

//    // If any change exceeds thresh, continue...
//    if ( bShouldStop && ( curVar - DiagGauss1DModel<Y>::getVariance(i) >  VARIANCE_THRESHOLD ||
//                          curVar - DiagGauss1DModel<Y>::getVariance(i) < -VARIANCE_THRESHOLD ) ) bShouldStop = false;

    // Avoid div by zero...
    if (DiagGauss1DModel<Y>::getVariance(i) < 1.0) DiagGauss1DModel<Y>::setVariance(i) = 1.0;

    //printf("cal variance i=%d var=%f\n", i, DiagGauss1DModel<Y>::getVariance(i));
  }
  DiagGauss1DModel<Y>::precomputeVarianceTerms();
}


#endif /*_NL_GAUSS__*/



