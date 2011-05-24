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


#ifndef _NL_DTREE_CONTIN__
#define _NL_DTREE_CONTIN__

#include "nl-dtree.h"

typedef double Wt;

////////////////////////////////////////////////////////////////////////////////
//
//  Cont DTree Node
//
////////////////////////////////////////////////////////////////////////////////

template<class Y, class P>
class ContDecisNode : public DecisNode<Y,P> {
 private:
  // Data members...
  Wt        wThreshold;     // Threshold weight ("w_0")
  map<A,Wt> awSeparator;    // Hyperplane separator, weights on each attribute/dimension
  Wt        wSumSqr;        // Sum of squares (parabolic) convolution coordinate weight

 public:
  // Constructor / destructor methods...
  ContDecisNode ( ) : wThreshold(0.0), wSumSqr(0.0) { }

  // Specification methods...
  Wt& setWt   ( )           { return wThreshold; }
  Wt& setWt   ( const A a ) { return (awSeparator.find(a)!=awSeparator.end()) ? awSeparator[a] : awSeparator[a]=0.0; }
  Wt& setSsWt ( )           { return wSumSqr; }

  // Extraction methods...
  const Wt getWt   ( )           const { return wThreshold; }
  const Wt getWt   ( const A a ) const { return ( (awSeparator.find(a)!=awSeparator.end()) ? awSeparator.find(a)->second : 0.0 ); }
  const Wt getSsWt ( )           const { return wSumSqr; }
};

////////////////////////////////////////////////////////////////////////////////
//
//  ContDTree Model
//
////////////////////////////////////////////////////////////////////////////////

template<class Y, class X, class P>
class ContDTree2DModel : public Generic2DModel<Y,X,P>, public Tree<ContDecisNode<Y,P> > {
 public:
  // Downcasts (safe b/c no new data)...
  ContDTree2DModel<Y,X,P>&       setLeft()        { return static_cast<ContDTree2DModel<Y,X,P>&> ( Tree<ContDecisNode<Y,P> >::setLeft()  ); }
  ContDTree2DModel<Y,X,P>&       setRight()       { return static_cast<ContDTree2DModel<Y,X,P>&> ( Tree<ContDecisNode<Y,P> >::setRight() ); }
  const ContDTree2DModel<Y,X,P>& getLeft()  const { return static_cast<const ContDTree2DModel<Y,X,P>&> ( Tree<ContDecisNode<Y,P> >::getLeft()  ); }
  const ContDTree2DModel<Y,X,P>& getRight() const { return static_cast<const ContDTree2DModel<Y,X,P>&> ( Tree<ContDecisNode<Y,P> >::getRight() ); }
  // Extraction methods...
  const P getProb ( const Y y, const X& x ) const {
    const Tree<ContDecisNode<Y,P> >* ptr = this;
      while ( !ptr->isTerm() ) { 
        double sumsqr=0.0;
        for(A a;a<X::getSize();a.setNext()) sumsqr += pow(x.get(a.toInt()),2.0) / X::getSize();
        Wt wtdavg = -Tree<ContDecisNode<Y,P> >::getWt();
        for(A a;a<X::getSize();a.setNext()) wtdavg += Tree<ContDecisNode<Y,P> >::getWt(a) * x.get(a.toInt());
        wtdavg += Tree<ContDecisNode<Y,P> >::getSsWt() * sumsqr;
        ptr = (wtdavg>0.0) ? &ptr->getRight() : &ptr->getLeft();
      }
    return ptr->getProb(y);
  }
  // Input / output methods...
  bool readFields  ( char*[], int ) ;
  void writeFields ( FILE* pf, string sPref ) {
    char psPath[1000] = "";
    write ( pf, (sPref+"").c_str(), psPath, 0 );
  }
  void write ( FILE* pf, const char psPrefix[], char psPath[], int iEnd ) const {
    if (Tree<ContDecisNode<Y,P> >::isTerm()) {
      Y y;
      psPath[iEnd]='\0';
      for ( bool b=y.setFirst(); b; b=y.setNext() )
        { fprintf(pf, "%s [%s] : ", psPrefix, psPath); y.write(pf); fprintf(pf, " = %f\n", (double)Tree<ContDecisNode<Y,P> >::getProb(y)); }
      ////psPath[iEnd]='\0'; fprintf ( pf, "%s [%s] : 0 = %f\n", psPrefix, psPath, (double)Tree<ContDecisNode<Y,P> >::getProb("0") );
      ////psPath[iEnd]='\0'; fprintf ( pf, "%s [%s] : 1 = %f\n", psPrefix, psPath, (double)Tree<ContDecisNode<Y,P> >::getProb("1") );
    } else {
        psPath[iEnd]='\0'; fprintf ( pf, "%s [%s] =", psPrefix, psPath );
        fprintf ( pf, " %f", Tree<ContDecisNode<Y,P> >::getWt() );
        for(A a;a<X::getSize();a.setNext()) fprintf ( pf, "_%f", Tree<ContDecisNode<Y,P> >::getWt(a.toInt()) );
        fprintf ( pf, "_%f", Tree<ContDecisNode<Y,P> >::getSsWt() );
        fprintf ( pf, "\n" );
      psPath[iEnd]='0'; psPath[iEnd+1]='\0'; getLeft().write  ( pf, psPrefix, psPath, iEnd+1 );
      psPath[iEnd]='1'; psPath[iEnd+1]='\0'; getRight().write ( pf, psPrefix, psPath, iEnd+1 );
    }
  }
};

////////////////////
template <class Y,class X, class P> 
bool ContDTree2DModel<Y,X,P>::readFields ( char* aps[], int numFields ) {
  if ( /*aps[0]==sId &&*/ (3==numFields || 4==numFields) ) {
    //fprintf(stderr,"%s,%d\n",aps[3],numFields);
    assert ( '['==aps[1][0] && ']'==aps[1][strlen(aps[1])-1] );

    // Start at root...
    Tree<ContDecisNode<Y,P> >* ptr = this;
    assert(ptr);

    // Find appropriate node, creating nodes as necessary...
    for(int i=1; i<strlen(aps[1])-1; i++) {
      assert ( '0'==aps[1][i] || '1'==aps[1][i] );
      ptr = ( ('0'==aps[1][i]) ? &ptr->setLeft() : &ptr->setRight() ) ;
      assert(ptr);
    }

    // Specify bit (at nonterminal) or distribution (at terminal)...
    if ( 3==numFields) {
      char* psT=NULL; Tree<ContDecisNode<Y,P> >::setWt() = atof(strtok_r(aps[2],"_",&psT));  ////atof(aps[2]);
      for(A a;a<X::getSize();a.setNext()) Tree<ContDecisNode<Y,P> >::setWt(a) = atof(strtok_r(NULL,"_",&psT));
      Tree<ContDecisNode<Y,P> >::setSsWt() = atof(strtok_r(NULL,"_",&psT)); }
      // atof(aps[3+a.toInt()]); }
    else if (4==numFields) ptr->setProb(aps[2]) = atof(aps[3]);
    else assert(false);

  } else return false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class Y, class X1,class X2, class P>
class ContDTree3DModel : public Generic3DModel<Y,X1,X2,P> {
 private:
  // Data members...
  string                               sId;
  SimpleHash<X1,ContDTree2DModel<Y,X2,P> > aqt;
 public:
  // Constructor / destructor methods...
  ContDTree3DModel ( )                 { }
  ContDTree3DModel ( const string& s ) { sId = s; }
  // Specification methods...
  ContDTree2DModel<Y,X2,P>& setTree ( const X1& x1 ) { return aqt.set(x1); }
  // Extraction methods...
  bool setFirst ( Y& y )                                  const { return y.setFirst(); }
  bool setNext  ( Y& y )                                  const { return y.setNext(); }
  P    getProb  ( const Y y, const X1& x1, const X2& x2 ) const { return aqt.get(x1).getProb(y,x2); }
  // Input / output methods...
  bool readFields  ( char*[], int ) ;
  void writeFields ( FILE* pf, string sPref ) {
    char psPath[1000] = "";
    X1 x1;
    for ( bool b=x1.setFirst(); b; b=x1.setNext() )
      aqt.get(x1).write ( pf, (sPref + " " + x1.getString()).c_str(), psPath, 0 );
  }
};

////////////////////
template <class Y,class X1,class X2, class P> 
bool ContDTree3DModel<Y,X1,X2,P>::readFields ( char* aps[], int numFields ) {
  if ( /*aps[0]==sId &&*/ (4==numFields || 5==numFields) ) {
    //fprintf(stderr,"%s,%d\n",aps[3],numFields);
    assert ( '['==aps[2][0] && ']'==aps[2][strlen(aps[2])-1] );

    // Start at root...
    Tree<ContDecisNode<Y,P> >* ptr = &aqt.set(aps[1]);
    assert(ptr);

    // Find appropriate node, creating nodes as necessary...
    for(int i=1; i<strlen(aps[2])-1; i++) {
      assert ( '0'==aps[2][i] || '1'==aps[2][i] );
      ptr = ( ('0'==aps[2][i]) ? &ptr->setLeft() : &ptr->setRight() ) ;
      assert(ptr);
    }

    // Specify bit (at nonterminal) or distribution (at terminal)...
    if ( 4==numFields) {
      char* psT=NULL;
      ptr->setWt() = atof(strtok_r(aps[3],"_",&psT));  ////atof(aps[3]);
      for(A a;a<X2::getSize();a.setNext()) ptr->setWt(a) = atof(strtok_r(NULL,"_",&psT));
      ptr->setSsWt() = atof(strtok_r(NULL,"_",&psT)); }
      ////for(A a;a<X2::getSize();a.setNext()) ptr->setWt(a) = atof(aps[4+a.toInt()]); }
    else if (5==numFields) ptr->setProb(aps[3]) = atof(aps[4]);
    ////    else if (5==numFields && 0==strcmp(aps[3],"0")) ptr->setProb()   = 1.0 - atof(aps[4]);
    ////    else if (5==numFields && 0==strcmp(aps[3],"1")) ptr->setProb()   = atof(aps[4]);
    else assert(false);

  } else return false;
  return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Trainable ContDTree Model
//
////////////////////////////////////////////////////////////////////////////////

template<class Y, class X, class P>
class TrainableContDTree2DModel : public ContDTree2DModel<Y,X,P> { 
 private:
  List<Joint2DRV<X,Y> > lxy;
 public:
  // Downcasts (safe b/c no new data)...
  TrainableContDTree2DModel<Y,X,P>&       setLeft()        { return static_cast<TrainableContDTree2DModel<Y,X,P>&>(ContDTree2DModel<Y,X,P>::setLeft());  }
  TrainableContDTree2DModel<Y,X,P>&       setRight()       { return static_cast<TrainableContDTree2DModel<Y,X,P>&>(ContDTree2DModel<Y,X,P>::setRight()); }
  const TrainableContDTree2DModel<Y,X,P>& getLeft()  const { return static_cast<const TrainableContDTree2DModel<Y,X,P>&>(ContDTree2DModel<Y,X,P>::getLeft());  }
  const TrainableContDTree2DModel<Y,X,P>& getRight() const { return static_cast<const TrainableContDTree2DModel<Y,X,P>&>(ContDTree2DModel<Y,X,P>::getRight()); }
  // Specification methods...
  void train ( List<Joint2DRV<X,Y> >&, const double ) ;
  void train ( const double d )                        { train(lxy,d); }
  ////// Input / output methods...
  bool readData ( char* vs[], int numFields ) { 
    if ( 3==numFields ) lxy.add() = Joint2DRV<X,Y> ( X(vs[1]), Y(vs[2]) );
    else return false;
    return true;
  }
};

////////////////////
template<class Y, class X, class P>
void  TrainableContDTree2DModel<Y,X,P>::train ( List<Joint2DRV<X,Y> >& lxy, const double DTREE_CHISQR_LIMIT ) {

  // Place to store counts...
  //CPT3DModel<A,B,Y,double> aaaCounts;  // hash was MUCH slower!!
  SafeArray2D<B,Y,double> aaCounts ( 2, Y::getDomain().getSize(), 0.0 );
  double dTot = lxy.getCard();
  CPT1DModel<Y,double> modelY;

//  if (11613==dTot) {  //if (12940<=dTot && dTot<12950) {  //if ( 20779==dTot ) { //// (bU)
//    ListedObject<Joint2DRV<X,Y> >* pxy;
//    for ( pxy = lxy.getFirst(); pxy; pxy = lxy.getNext(pxy) ) {
//      fprintf(stdout,"Ohist "); pxy->getSub1().write(stdout); fprintf(stdout," "); pxy->getSub2().write(stdout); fprintf(stdout,"\n");
//    }
//    fprintf(stderr,"PRINTED\n");
//  }

  // For each datum in list...
  ListedObject<Joint2DRV<X,Y> >* pxy;
  for ( pxy = lxy.getFirst(); pxy; pxy = lxy.getNext(pxy) ) {
    // Count Ys...
    modelY.setProb(pxy->getSub2())++;
  }
  modelY.normalize();

  double prRarest = (modelY.getProb("1")<modelY.getProb("0")) ? modelY.getProb("1") : modelY.getProb("0");

//  // Set separator to pass through center of positives...
//  for ( pxy = lxy.getFirst(); pxy; pxy = lxy.getNext(pxy) )
//    for ( A a; a<X::getSize(); a.setNext() ) {
//      if ( Y("1")==pxy->getSub2() ) {
//        Tree<ContDecisNode<Y,P> >::setWt()   -= (pxy->getSub1().get(a.toInt())+pow(pxy->getSub1().get(a.toInt()),2.0))/dTot; //// (dTot*prRarest);
//        Tree<ContDecisNode<Y,P> >::setWt(a)  += pxy->getSub1().get(a.toInt())/dTot;  //// (dTot*prRarest);
//        Tree<ContDecisNode<Y,P> >::setSsWt() += pow(pxy->getSub1().get(a.toInt()),2.0)/dTot; //// (dTot*prRarest);
//      }
//    }

  // Set separator to pass through center of positives...
  Tree<ContDecisNode<Y,P> >::setWt() = 1.0;


  // For each gradient descent epoch...
  for ( int epoch=1; epoch<=1000; epoch++ ) {

    double dCtr=0.0;

    double dPos     = 0.0;
    ListedObject<Joint2DRV<X,Y> >* pxy;

    if(OUTPUT_NOISY) {
      double lgprTot = 0.0;
//      // For each datum in list...
//      for ( pxy = lxy.getFirst(); pxy; pxy = lxy.getNext(pxy) ) {
//        // Calc tot prob...
//        double wtdavg = -Tree<ContDecisNode<Y,P> >::getWt();
//        for(A a;a<X::getSize();a.setNext()) wtdavg += Tree<ContDecisNode<Y,P> >::getWt(a) * pxy->getSub1().get(a.toInt());
//        // Calc est val of Y using sigmoid transfer fn...
//        P prY = 1.0 / ( 1.0 + exp(-wtdavg) );
//        if(epoch>1)fprintf(stderr,"    %f %f\n",(double)wtdavg,(double)prY);
//        lgprTot += (pxy->getSub2()==1) ? log(prY) : log(1.0-prY) ;
//      }

      if (OUTPUT_NOISY && epoch%10==0) {
        // Report...
        fprintf(stderr,"  tot=%08d totlogprob=%g separator=%f",(int)dTot,lgprTot,Tree<ContDecisNode<Y,P> >::getWt());
        for(A a;a<X::getSize();a.setNext()) fprintf(stderr,"_%f",Tree<ContDecisNode<Y,P> >::getWt(a));
        fprintf(stderr,"_%f",Tree<ContDecisNode<Y,P> >::getSsWt());
        fprintf(stderr,"\n");
      }
    }

    fprintf(stderr,"  --- epoch %d ---\n",epoch);

    // For each datum in list...
    for ( pxy = lxy.getFirst(); pxy; pxy = lxy.getNext(pxy) ) {
//    // Use random subset of more frequent Y val so total wts for 1 and 0 are equal (CODE REVIEW: should be subset nearest to centroid of fewer)...
//    if ( double(rand())/double(RAND_MAX) < prRarest/modelY.getProb(pxy->getSub2()) ) {

        dCtr++;
        double gamma = dTot/(dTot+dCtr); // 1.0/(double(epoch)+dCtr/dTot); // 1.0/double(epoch); // 1.0/(double(epoch)+dCtr/(dTot*prRarest*2.0)); // 

        // Weight deltas for next epoch...
        Wt wDelta = 0.0;
        SafeArray1D<A,Wt> awDeltas (X::getSize(),0.0);
        Wt wSsDelta = 0.0;

        // Calc sum of squares for convolution coordinate...
        double sumsqr=0.0;
        for(A a;a<X::getSize();a.setNext()) sumsqr += pow(pxy->getSub1().get(a.toInt()),2.0) / X::getSize();

        // Calc wtd avg of feats...
        double wtdavg = -Tree<ContDecisNode<Y,P> >::getWt();
        for(A a;a<X::getSize();a.setNext()) wtdavg += Tree<ContDecisNode<Y,P> >::getWt(a) * pxy->getSub1().get(a.toInt());
        wtdavg += Tree<ContDecisNode<Y,P> >::getSsWt() * sumsqr;
        //// Calc est val of Y using sigmoid transfer fn...
        //P prY = ( ( ( (1.0/(1.0+exp(-wtdavg))) - .5 ) * exp(-wtdavg) ) + .5 ) ;
        // Calc est val of Y using sigmoid transfer fn...
        P prY = 1.0 / ( 1.0 + exp(-wtdavg) );

        // Calc deltas for each feature/attribute/dimension...
        double dEachWt  = 1.0/dTot;  // 1.0/dTot * modelY.getProb ( Y(1-pxy->getSub2().toInt()) );  // 1.0/(dTot*prRarest*2.0); // 
        wDelta += dEachWt * -1 * ( prY - P(double(pxy->getSub2().toInt())) );
        for ( A a; a<X::getSize(); a.setNext() )
          awDeltas.set(a) += dEachWt * pxy->getSub1().get(a.toInt()) * ( prY - P(double(pxy->getSub2().toInt())) );
        wSsDelta += dEachWt * sumsqr * ( prY - P(double(pxy->getSub2().toInt())) );

        // Update weights by deltas...
        //Tree<ContDecisNode<Y,P> >::setWt() -= gamma * wDelta;
        ////double reldeduction = wDelta/Tree<ContDecisNode<Y,P> >::getWt();
        for ( A a; a<X::getSize(); a.setNext() )
          Tree<ContDecisNode<Y,P> >::setWt(a) -= gamma*awDeltas.get(a); //+ changeratio/Tree<ContDecisNode<Y,P> >::getWt(a);
        Tree<ContDecisNode<Y,P> >::setSsWt()  -= gamma*wSsDelta;        //+ changeratio/Tree<ContDecisNode<Y,P> >::getSsWt();

        dPos+=prY;  //      if (prY>0.5) dPos++;

        // Report...
        if(OUTPUT_VERYNOISY) {
          fprintf(stderr,"    A tot=%08d vals = %f",(int)dTot,-1.00);
          for(A a;a<X::getSize();a.setNext()) fprintf(stderr,"_%f",pxy->getSub1().get(a.toInt()));
          fprintf(stderr,"_%f",sumsqr);
          fprintf(stderr,"  --> %f %f (gold: %f)\n",wtdavg,(double)prY,double(pxy->getSub2().toInt()));
          fprintf(stderr,"    D tot=%08d delt = %f",(int)dTot,wDelta);
          for(A a;a<X::getSize();a.setNext()) fprintf(stderr,"_%f",awDeltas.get(a));
          fprintf(stderr,"_%f",wSsDelta);
          fprintf(stderr,"\n");
        }

        // Report...
        if(OUTPUT_VERYNOISY) {
          fprintf(stderr,"   _S tot=%08d sepr = %f",(int)dTot,Tree<ContDecisNode<Y,P> >::getWt());
          for(A a;a<X::getSize();a.setNext()) fprintf(stderr,"_%f",Tree<ContDecisNode<Y,P> >::getWt(a));
          fprintf(stderr,"_%f",Tree<ContDecisNode<Y,P> >::getSsWt());
          fprintf(stderr,"\n");
        }
//    }
    } // end loop pxy

/*     // Report... */
/*     if(OUTPUT_NOISY) { */
/*       fprintf(stderr,"  tot:%08d +:%08d -:%08d\n",(int)dTot,(int)dPos,(int)(dTot-dPos)); */
/*       fprintf(stderr,"  E tot=%08d separator=%f",(int)dTot,wDelta); */
/*       for(A a;a<X::getSize();a.setNext()) fprintf(stderr,"_%f",awDeltas.get(a)); */
/*       fprintf(stderr,"\n"); */
/*     } */
  } // end loop epoch

  // Split list into each 0/1 child of this node...
  List<Joint2DRV<X,Y> > alxy[2];
  int                   actr[2] = {0,0};
  // For each datum in list...
  while ( !lxy.isEmpty() ) {
    Joint2DRV<X,Y>* pxy = lxy.getFirst();
    double sumsqr=0.0;
    for(A a;a<X::getSize();a.setNext()) sumsqr += pow(pxy->getSub1().get(a.toInt()),2.0) / X::getSize();
    Wt wtdavg=-Tree<ContDecisNode<Y,P> >::getWt();
    for(A a;a<X::getSize();a.setNext()) wtdavg += Tree<ContDecisNode<Y,P> >::getWt(a) * pxy->getSub1().get(a.toInt());
    wtdavg += Tree<ContDecisNode<Y,P> >::getSsWt() * sumsqr;
    alxy[(wtdavg>0.0)?1:0].add() = *pxy;
    aaCounts.set((wtdavg>0.0)?1:0,pxy->getSub2())++;
    actr[(wtdavg>0.0)?1:0]++;
    if(OUTPUT_VERYNOISY){fprintf(stderr,"classify "); pxy->write(stderr); fprintf(stderr," wtdavg=%f class=%d\n",wtdavg,(wtdavg>0.0)?1:0);}
    lxy.pop();
  }

  // Calc chisqr...
  double chisqr = 0.0;
  fprintf(stderr,"    tot=%08d split=",(int)dTot);
  for ( int b=0; b<2; b++ ) {
    Y y;
    for ( bool by=y.setFirst(); by; by=y.setNext() ) {
      fprintf(stderr," (%s->%d:%f)",y.getString().c_str(),b,aaCounts.get(b,y));
      if ( actr[b]>0.0 && modelY.getProb(y)>0.0 && dTot>0.0 ) {
        double expect = actr[b] * modelY.getProb(y);
        chisqr += pow ( aaCounts.get(b,y)-expect, 2.0 ) / expect;
      }
    }
  }
  fprintf(stderr,"\n");

  // Report...
  if(OUTPUT_NOISY) {
    fprintf(stderr,"  tot=%08d separator=%f",(int)dTot,Tree<ContDecisNode<Y,P> >::getWt());
    for(A a;a<X::getSize();a.setNext()) fprintf(stderr,"_%f",Tree<ContDecisNode<Y,P> >::getWt(a));
    fprintf(stderr,"_%f",Tree<ContDecisNode<Y,P> >::getSsWt());
    fprintf(stderr," chisqr=%g\n",chisqr);
  }

  // If separation is significant to chisqr limit...
  if ( chisqr > DTREE_CHISQR_LIMIT ) {
    // Recursively call train at each child...
    setRight().train ( alxy[1], DTREE_CHISQR_LIMIT );  ////node*2LL+1LL);
    setLeft().train  ( alxy[0], DTREE_CHISQR_LIMIT );  ////node*2LL);
  }
  // If separation is not significant...
  else {
    // Add ratio as leaf...
    Y y;
    for ( bool by=y.setFirst(); by; by=y.setNext() )
      ContDecisNode<Y,P>::setProb(y) = (dTot>0.0) ? modelY.getProb(y) : 1.0/Y::getDomain().getSize();
  }
}


////////////////////////////////////////////////////////////////////////////////

template<class Y, class X1, class X2, class P>
class TrainableContDTree3DModel : public ContDTree3DModel<Y,X1,X2,P> { 

 private:

  map<X1,List<Joint2DRV<X2,Y> > > mqlxy;

 public:

  ////// Constructor...
  TrainableContDTree3DModel()               { }
  TrainableContDTree3DModel(const char* ps) : ContDTree3DModel<Y,X1,X2,P>(ps) { }

  ////// setTree downcast...
  TrainableContDTree2DModel<Y,X2,P>& setTree(const X1& x1) { return static_cast<TrainableContDTree2DModel<Y,X2,P>&>(ContDTree3DModel<Y,X1,X2,P>::setTree(x1)); }

  ////// Add training data to per-subphone lists...
  bool readData ( char* vs[], int numFields ) { 
    if ( 4==numFields ) {
      mqlxy[X1(vs[1])].add() = Joint2DRV<X2,Y> ( X2(vs[2]), Y(vs[3]) );
      ////mqlxy[X1(vs[1])].getLast()->write(stderr); fprintf(stderr,"\n");
    }
    else return false;
    return true;
  }

  ////// Train each subphone...
  void train ( const double DTREE_CHISQR_LIMIT ) {
    int ctr = 0;
    // For each subphone...
    X1 x1; for ( bool b=x1.setFirst(); b; b=x1.setNext() ) {
      if(OUTPUT_NOISY)
        fprintf(stderr,"***** x1:%s (number %d) *****\n",x1.getString().c_str(),ctr++);
      setTree(x1).train ( mqlxy[x1], DTREE_CHISQR_LIMIT );
    }
  }
};

#endif // _NL_DTREE_CONTIN__
