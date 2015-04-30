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

#ifndef _NL_CRF__
#define _NL_CRF__

#include "nl-safeids.h"
#include "nl-probmodel.h"
#include <cassert>
#include <math.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  CRFModeledRV<Y>
//
////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2>
class CRF3DModeledRV : public Y {

 private:

  // Static data members...
  static bool               bModel;      // whether model defined yet
  static int                cardGlb;     // global dependencies (used in all potentials)
  static int                cardOff;     // offset positions in site var sequence
  static int                cardSh;      // clique shapes at each offset
  static int                cardCnd;     // possible condition clique configs incl non-site vars in high bits
  static int                bitsVal;     // size in bits of val part of clique config
  static int                bitsValSite; // size in bits of each site var in val clique config
  static SafeArray5D<Id<int>,int,int,int,int,float> aaaaaPotentials;  // the model
/*   static SafeArray3D<int>   aaaCnds;          // calc features only once per frame */

 public:

  // Static extraction methods...
  static const float& getPotential ( Id<int> glb, int off, int sh, int configCnd, int configVal )
    { assert(bModel); return aaaaaPotentials.set(glb,off,sh,configCnd,configVal); }

  // Static specification methods...
  static void   init         ( int g, int o, int s, int c, int v, int b )
    { cardGlb=g; cardOff=o; cardSh=s; cardCnd=c; bitsVal=v; bitsValSite=b; }
  static float& setPotential ( Id<int> glb, int off, int sh, int configCnd, int configVal )
    { if(!bModel){aaaaaPotentials.init(cardGlb,cardOff,cardSh,cardCnd,1<<bitsVal,1.0); bModel=true;}
      return aaaaaPotentials.set(glb,off,sh,configCnd,configVal); }
  static void updateObservCliques ( const X1&, const X2& ) ;

  // Static input / output methods...
  static bool readModelFields ( char*[], int ) ;

  // Extraction methods...
  Prob getProb ( const X1&, const X2& ) const ;

  // Input / output methods...
  void writeObservCliqueConfigs ( FILE*, int, const char*, const X1&, const X2&, bool ) const ;
};

////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2> bool               CRF3DModeledRV<Y,X1,X2>::bModel      = false;
template <class Y,class X1,class X2> int                CRF3DModeledRV<Y,X1,X2>::cardGlb     = 0;
template <class Y,class X1,class X2> int                CRF3DModeledRV<Y,X1,X2>::cardOff     = 0;
template <class Y,class X1,class X2> int                CRF3DModeledRV<Y,X1,X2>::cardSh      = 0;
template <class Y,class X1,class X2> int                CRF3DModeledRV<Y,X1,X2>::cardCnd     = 0;
template <class Y,class X1,class X2> int                CRF3DModeledRV<Y,X1,X2>::bitsVal     = 0;
template <class Y,class X1,class X2> int                CRF3DModeledRV<Y,X1,X2>::bitsValSite = 0;
template <class Y,class X1,class X2> SafeArray5D<Id<int>,int,int,int,int,float> CRF3DModeledRV<Y,X1,X2>::aaaaaPotentials;
/* template <class Y,class X1,class X2> SafeArray3D<int>   CRF3DModeledRV<Y,X1,X2>::aaaCnds; */

////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2>
Prob CRF3DModeledRV<Y,X1,X2>::getProb( const X1& x1, const X2& x2 ) const {

  SafeArray2D<int,int,int>    aaCnds  ( cardOff, cardSh ) ;
  SafeArray2D<int,int,double> aaTrell ( cardOff, 1<<bitsVal, 0.0 ) ;
  double prob = 1.0;

  // For each offset...
  for ( int off=0; off<cardOff; off++ )
    // For each shape...
    for ( int sh=0; sh<cardSh; sh++ )
      // Update clique config for condition...
      aaCnds.set(off,sh) = Y::getCliqueConfigCnd ( x1, x2, off, sh ) ;

  // For each offset...
  for ( int off=0; off<cardOff; off++ ) {
    // For each shape...
    for ( int sh=0; sh<cardSh; sh++ )
      // Multiply phi for feature (that is, exp lambda) into numerator...
      prob *= getPotential(Y::getGlobalDependency(x1,x2),off,sh,
                           aaCnds.get(off,sh),
                           Y::getCliqueConfigVal(off,sh));

    // If first column in trellis...
    if ( 0==off ) {
      // For each trellis value...
      for ( int configVal=0; configVal<(1<<bitsVal); configVal++ ) {
        // Add weight of each shape at current offset...
        float prod=1.0;
        for ( int sh=0; sh<cardSh; sh++ )
          prod *= getPotential(Y::getGlobalDependency(x1,x2),off,sh,
                               aaCnds.get(off,sh),
                               configVal) ;
        aaTrell.set(off,configVal) = prod ;
      }
    // If subsequent column in trellis...
    } else {
      // For each trellis transition (overlap = all but one)...
      for ( int configRghtValSite=0; configRghtValSite<(1<<bitsValSite); configRghtValSite++ )
        for ( int configValOverlap=0; configValOverlap<(1<<(bitsVal-bitsValSite)); configValOverlap++ ) {
          int configRghtVal = (configValOverlap<<bitsValSite)+configRghtValSite;
          // For each possible preceding trellis node...
          for ( int configLeftValSite=0; configLeftValSite<(1<<bitsValSite); configLeftValSite++ ) {
            int configLeftVal = (configLeftValSite<<(bitsVal-bitsValSite))+configValOverlap;
            // Add product of result and previous trellis cell to current trellis cell...
            aaTrell.set(off,configRghtVal) += aaTrell.get(off-1,configLeftVal) ;
          }
          // Multiply weight of each shape...
          float prod=1.0;
          for ( int sh=0; sh<cardSh; sh++ )
            prod *= getPotential(Y::getGlobalDependency(x1,x2),off,sh,
                                 aaCnds.get(off,sh),
                                 configRghtVal);
          aaTrell.set(off,configRghtVal) *= prod;
        }
    }
  } // END EACH OFFSET

  // Calc total prob mass: sum of all possible forward scores in trellis...
  double probZ = 0.0;
  for ( int i=0; i<(1<<bitsVal); i++ )
    probZ += aaTrell.get(cardOff-1,i);
  // Normalize prob by total prob mass...
  return prob/probZ;
}

////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2>
bool CRF3DModeledRV<Y,X1,X2>::readModelFields ( char* aps[], int numFields ) {
  if ( 7==numFields )
    setPotential ( X1(string(aps[1])),                  // globals
                   atoi(aps[2]),                        // offsets
                   atoi(aps[3]),                        // shapes
                   atoi(aps[4]),                        // cnds
                   atoi(aps[5]) ) = exp(atof(aps[6])) ; // vals
  else return false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2>
void CRF3DModeledRV<Y,X1,X2>::writeObservCliqueConfigs ( FILE* pf, int frame, const char* psMdl,
                                                         const X1& x1, const X2& x2, bool bObsVal ) const {
  fprintf ( pf, "%04d> %s ", frame, psMdl );
  // For each shape (feature slope)...
  for ( int sh=0; sh<cardSh; sh++ ) {
    // Print clique config condition at each offset...
    for ( int off=0; off<cardOff; off++ )
      fprintf ( pf, "%c", intToTetraHex(Y::getCliqueConfigCnd(x1,x2,off,sh)) );
    if (sh<cardSh-1) printf(",");   // commas between shapes
  }
  printf(" : ");  // cond/val delimiter
  // Print clique config value at each offset...
  if ( bObsVal )
    for ( int off=0; off<cardOff; off++ )
      fprintf ( pf, "%c", intToTetraHex(Y::getCliqueConfigVal(off,0)) );
  else fprintf ( pf, "_" ) ;
  printf("\n");
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//  CRF4DModeledRV<Y>
//
////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2,class X3>
class CRF4DModeledRV : public Y {

 private:

  // Static data members...
  static bool               bModel;      // whether model defined yet
  static int                cardGlb;     // global dependencies (used in all potentials)
  static int                cardOff;     // offset positions in site var sequence
  static int                cardSh;      // clique shapes at each offset
  static int                cardCnd;     // possible condition clique configs incl non-site vars in high bits
  static int                bitsVal;     // size in bits of val part of clique config
  static int                bitsValSite; // size in bits of each site var in val clique config
  static SafeArray5D<Id<int>,int,int,int,int,float> aaaaaPotentials;  // the model
/*   static SafeArray3D<int>   aaaCnds;          // calc features only once per frame */

 public:

  // Static extraction methods...
  static const float& getPotential ( Id<int> glb, int off, int sh, int configCnd, int configVal )
    { assert(bModel); return aaaaaPotentials.set(glb,off,sh,configCnd,configVal); }

  // Static specification methods...
  static void   init         ( int g, int o, int s, int c, int v, int b )
    { cardGlb=g; cardOff=o; cardSh=s; cardCnd=c; bitsVal=v; bitsValSite=b; }
  static float& setPotential ( Id<int> glb, int off, int sh, int configCnd, int configVal )
    { if(!bModel){aaaaaPotentials.init(cardGlb,cardOff,cardSh,cardCnd,1<<bitsVal,1.0); bModel=true;}
      return aaaaaPotentials.set(glb,off,sh,configCnd,configVal); }

  // Static input / output methods...
  static bool readModelFields ( char*[], int ) ;

  // Extraction methods...
  Prob getProb ( const X1&, const X2&, const X3& ) const ;

  // Input / output methods...
  void writeObservCliqueConfigs ( FILE*, int, const char*, const X1&, const X2&, const X3&, bool ) const ;
};

////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2,class X3> bool CRF4DModeledRV<Y,X1,X2,X3>::bModel   = false;
template <class Y,class X1,class X2,class X3> int  CRF4DModeledRV<Y,X1,X2,X3>::cardGlb     = 0;
template <class Y,class X1,class X2,class X3> int  CRF4DModeledRV<Y,X1,X2,X3>::cardOff     = 0;
template <class Y,class X1,class X2,class X3> int  CRF4DModeledRV<Y,X1,X2,X3>::cardSh      = 0;
template <class Y,class X1,class X2,class X3> int  CRF4DModeledRV<Y,X1,X2,X3>::cardCnd     = 0;
template <class Y,class X1,class X2,class X3> int  CRF4DModeledRV<Y,X1,X2,X3>::bitsVal     = 0;
template <class Y,class X1,class X2,class X3> int  CRF4DModeledRV<Y,X1,X2,X3>::bitsValSite = 0;
template <class Y,class X1,class X2,class X3> SafeArray5D<Id<int>,int,int,int,int,float>
               CRF4DModeledRV<Y,X1,X2,X3>::aaaaaPotentials;
/* template <class Y,class X1,class X2> SafeArray3D<int>   CRF4DModeledRV<Y,X1,X2>::aaaCnds; */

////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2,class X3>
Prob CRF4DModeledRV<Y,X1,X2,X3>::getProb( const X1& x1, const X2& x2, const X3& x3 ) const {

  SafeArray2D<int,int,int>    aaCnds  ( cardOff, cardSh ) ;
  SafeArray2D<int,int,double> aaTrell ( cardOff, 1<<bitsVal, 0.0 ) ;
  double prob = 1.0;

  // For each offset...
  for ( int off=0; off<cardOff; off++ )
    // For each shape...
    for ( int sh=0; sh<cardSh; sh++ )
      // Update clique config for condition...
      aaCnds.set(off,sh) = Y::getCliqueConfigCnd ( x1, x2, x3, off, sh ) ;

  // For each offset...
  for ( int off=0; off<cardOff; off++ ) {
    // For each shape...
    for ( int sh=0; sh<cardSh; sh++ )
      // Multiply phi for feature (that is, exp lambda) into numerator...
      prob *= getPotential(Y::getGlobalDependency(x1,x2,x3),off,sh,
                           aaCnds.get(off,sh),
                           Y::getCliqueConfigVal(off,sh));

    // If first column in trellis...
    if ( 0==off ) {
      // For each trellis value...
      for ( int configVal=0; configVal<(1<<bitsVal); configVal++ ) {
        // Add weight of each shape at current offset...
        float prod=1.0;
        for ( int sh=0; sh<cardSh; sh++ )
          prod *= getPotential(Y::getGlobalDependency(x1,x2,x3),off,sh,
                               aaCnds.get(off,sh),
                               configVal) ;
        aaTrell.set(off,configVal) = prod ;
      }
    // If subsequent column in trellis...
    } else {
      // For each trellis transition (overlap = all but one)...
      for ( int configRghtValSite=0; configRghtValSite<(1<<bitsValSite); configRghtValSite++ )
        for ( int configValOverlap=0; configValOverlap<(1<<(bitsVal-bitsValSite)); configValOverlap++ ) {
          int configRghtVal = (configValOverlap<<bitsValSite)+configRghtValSite;
          // For each possible preceding trellis node...
          for ( int configLeftValSite=0; configLeftValSite<(1<<bitsValSite); configLeftValSite++ ) {
            int configLeftVal = (configLeftValSite<<(bitsVal-bitsValSite))+configValOverlap;
            // Add product of result and previous trellis cell to current trellis cell...
            aaTrell.set(off,configRghtVal) += aaTrell.get(off-1,configLeftVal) ;
          }
          // Multiply weight of each shape...
          float prod=1.0;
          for ( int sh=0; sh<cardSh; sh++ )
            prod *= getPotential(Y::getGlobalDependency(x1,x2,x3),off,sh,
                                 aaCnds.get(off,sh),
                                 configRghtVal);
          aaTrell.set(off,configRghtVal) *= prod;
        }
    }
  } // END EACH OFFSET

  // Calc total prob mass: sum of all possible forward scores in trellis...
  double probZ = 0.0;
  for ( int i=0; i<(1<<bitsVal); i++ )
    probZ += aaTrell.get(cardOff-1,i);
  // Normalize prob by total prob mass...
  return prob/probZ;
}

////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2,class X3>
bool CRF4DModeledRV<Y,X1,X2,X3>::readModelFields ( char* aps[], int numFields ) {
  if ( 7==numFields )
    setPotential ( X1(string(aps[1])),                  // globals
                   atoi(aps[2]),                        // offsets
                   atoi(aps[3]),                        // shapes
                   atoi(aps[4]),                        // cnds
                   atoi(aps[5]) ) = exp(atof(aps[6])) ; // vals
  else return false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////

template <class Y,class X1,class X2, class X3>
void CRF4DModeledRV<Y,X1,X2,X3>::writeObservCliqueConfigs ( FILE* pf, int frame, const char* psMdl,
                                                            const X1& x1, const X2& x2,
                                                            const X3& x3, bool bObsVal ) const {
  fprintf ( pf, "%04d> %s ", frame, psMdl );
  // For each shape (feature slope)...
  for ( int sh=0; sh<cardSh; sh++ ) {
    // Print clique config condition at each offset...
    for ( int off=0; off<cardOff; off++ )
      fprintf ( pf, "%c", intToTetraHex(Y::getCliqueConfigCnd(x1,x2,x3,off,sh)) );
    if (sh<cardSh-1) printf(",");   // commas between shapes
  }
  printf(" : ");  // cond/val delimiter
  // Print clique config value at each offset...
  if ( bObsVal )
    for ( int off=0; off<cardOff; off++ )
      fprintf ( pf, "%c", intToTetraHex(Y::getCliqueConfigVal(off,0)) );
  else fprintf ( pf, "_" ) ;
  printf("\n");
}

#endif //_NL_CRF__
