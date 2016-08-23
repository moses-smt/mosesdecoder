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

#ifndef _NL_DTREE__
#define _NL_DTREE__

#include <math.h>
#include <vector>
#include <string>
#include <map>
#include "nl-safeids.h"
//#include "nl-probmodel.h"
#include "nl-cpt.h"
#include "nl-randvar.h"
#include "nl-tree.h"
#include "nl-hash.h"
#include "nl-list.h"

//#define UNIGR_FO .5  // IF NO EXAMPLES, USE APRIORI PROB OF FO

//typedef Id<int>  A; //BitNum;
// //typedef Id<char> B;

using namespace std;

////////////////////////////////////////////////////////////////////////////////
//
//  DTree Node
//
////////////////////////////////////////////////////////////////////////////////

template<class X, class Y, class P>
class DecisNode {
 public:
  // Public types...
  typedef Id<int>  A;

 private:
  // Private types...
  typedef typename X::ElementType B;
  // Data members...
  A        aNontermDecis;  // Nonterminal nodes have an attribute (e.g. convexity bit) on which to condition
  map<Y,P> aprTermDistrib; // Terminal nodes have a distribution over Y values

 public:
  // Constructor / destructor methods...
  DecisNode ( ) : aNontermDecis(-1) { }

  // Specification methods...
  A&  setA    ( )           { return aNontermDecis;     }
  P&  setProb ( const Y y ) { return aprTermDistrib[y]; }

  // Extraction methods...
  const A  getA    ( )           const { return aNontermDecis;    }
  const P  getProb ( const Y y ) const { return ( (aprTermDistrib.empty()) ? P(1.0/Y::getDomain().getSize()) :
                                                 (aprTermDistrib.find(y)!=aprTermDistrib.end()) ? aprTermDistrib.find(y)->second : P() ); }
};


////////////////////////////////////////////////////////////////////////////////
//
//  DTree Model
//
////////////////////////////////////////////////////////////////////////////////

template<class Y, class X, class P>
//class DTree2DModel : public Generic2DModel<Y,X,P>, public Tree < typename X::ElementType, DecisNode<X,Y,P> > {
class DTree2DModel : public Tree < typename X::ElementType, DecisNode<X,Y,P> > {
 private:
  // Type members...
  typedef typename X::ElementType B;
 public:
  // Downcasts (safe b/c no new data)...
  DTree2DModel<Y,X,P>&       setBranch(const B& b)        { return static_cast<DTree2DModel<Y,X,P>&>       ( Tree<B,DecisNode<X,Y,P> >::setBranch(b) ); }
  const DTree2DModel<Y,X,P>& getBranch(const B& b)  const { return static_cast<const DTree2DModel<Y,X,P>&> ( Tree<B,DecisNode<X,Y,P> >::getBranch(b) ); }
  // Extraction methods...
  const P getProb ( const Y y, const X& x ) const {
    const Tree<B,DecisNode<X,Y,P> >* ptr = this;
    while ( !ptr->isTerm() ) /*{cerr<<x.get(ptr->getA().toInt())<<",";*/ ptr = &ptr->getBranch ( x.get(ptr->getA().toInt()) ); /*}*/
    return ptr->getProb(y);
  }
  // Input / output methods...
  bool readFields  ( Array<char*>& ) ;
  void write ( FILE* pf, const char psPrefix[], char psPath[], int iEnd ) const {
    if (Tree<B,DecisNode<X,Y,P> >::isTerm()) {
      psPath[iEnd]='\0';
      Y y;
      for ( bool b=y.setFirst(); b; b=y.setNext() )
        { fprintf(pf, "%s [%s] : ", psPrefix, psPath); fprintf(pf,"%s",y.getString().c_str()); fprintf(pf, " = %f\n", Tree<B,DecisNode<X,Y,P> >::getProb(y).toDouble()); }
      ////psPath[iEnd]='\0'; fprintf ( pf, "%s [%s] : 0 = %f\n", psPrefix, psPath, (double)Tree<DecisNode<Y,P> >::getProb("0") );
      ////psPath[iEnd]='\0'; fprintf ( pf, "%s [%s] : 1 = %f\n", psPrefix, psPath, (double)Tree<DecisNode<Y,P> >::getProb("1") );
    } else {
      psPath[iEnd]='\0'; fprintf ( pf, "%s [%s] = %d\n", psPrefix, psPath, (int)Tree<B,DecisNode<X,Y,P> >::getA().toInt() );
      B b;
      for ( bool bb=b.setFirst(); bb; bb=b.setNext() ) {
        psPath[iEnd]=b.getString().c_str()[0]; psPath[iEnd+1]='\0'; getBranch(b).write(pf,psPrefix,psPath,iEnd+1);
//      psPath[iEnd]='0'; psPath[iEnd+1]='\0'; getLeft().write  ( pf, psPrefix, psPath, iEnd+1 );
//      psPath[iEnd]='1'; psPath[iEnd+1]='\0'; getRight().write ( pf, psPrefix, psPath, iEnd+1 );
      }
    }
  }
  void writeFields ( FILE* pf, string sPref ) {
    char psPath[1000] = "";
    write ( pf, (sPref+"").c_str(), psPath, 0 );
  }
  ////
  friend pair<StringInput,DTree2DModel<Y,X,P>*> operator>> ( StringInput si, DTree2DModel<Y,X,P>& m ) {
    return pair<StringInput,DTree2DModel<Y,X,P>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,DTree2DModel<Y,X,P>*> si_m, const char* psD ) {
    if (StringInput(NULL)==si_m.first) return si_m.first;
    Y y; String xs; StringInput si,si2; si=si_m.first; DTree2DModel<Y,X,P>* pm=si_m.second;
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>xs>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    // Find appropriate node, creating nodes as necessary...
    for(int i=1; i<int(strlen(xs.c_array()))-1; i++) {
      char psTemp[2]=" "; psTemp[0]=xs.c_array()[i];
      pm = &pm->setBranch ( B(psTemp) );
    }

    if ( si!=NULL && si[0]==':' ) {
      si=si>>": ";
      while((si2=si>>" ")!=NULL)si=si2;
      si=si>>y>>" ";
      while((si2=si>>" ")!=NULL)si=si2;
      si=si>>"= ";
      while((si2=si>>" ")!=NULL)si=si2;
      // Specify attribute number (at nonterminal) or probability in distribution (at terminal)...
      return (si!=NULL) ? si>>pm->setProb(y)>>psD : si;
    }
    else if ( si!=NULL && si[0]=='=' ) {
      si=si>>"= "; //cerr<<" in after equals "<<((si==NULL) ? "yes" : "no") << endl;
      while((si2=si>>" ")!=NULL)si=si2;

      //m.setA() = atoi(si.c_str());
      int aVar = 0;
      si=si>>aVar>>psD;
      pm->setA()=aVar;
      ////cerr<<" at end "<<((si==NULL) ? "yes" : "no") << endl;
      ////cerr<<"  m.getA() is "<< m.getA().toInt() << endl;
      return si;
      //return (si!=NULL) ? si>>m.setA()>>psD : si;
    }
    else if ( si!=NULL ) cerr<<" ??? ["<<si.c_str()<<"]\n";
    return StringInput(NULL);

    /*
    Y y; String xs; X x; StringInput si,si2; string sRt; DTree2DModel<Y,X,P>& m = *si_m.second;
    si=si_m.first;
    sRt = si.c_str();
    if (sRt.find(':')!=string::npos) {
      while((si2=si>>" [")!=NULL)si=si2;
      si=si>>xs>>"] ";
      while((si2=si>>" ")!=NULL)si=si2;
      si=si>>": ";
      while((si2=si>>" ")!=NULL)si=si2;
      si=si>>y>>" ";
      while((si2=si>>" ")!=NULL)si=si2;
      si=si>>"= ";

      // For DTree, must find the node labeled by X
      //Tree<B,DecisNode<X,Y,P> >* ptr = m;
      //assert(ptr);
      // Find appropriate node, creating nodes as necessary...
      for(int i=1; i<int(strlen(xs.c_array()))-1; i++) {
	char psTemp[2]="\0"; psTemp[0]=xs.c_array()[i];
	m = m.setBranch ( B(psTemp) );
      }
      // Specify attribute number (at nonterminal) or probability in distribution (at terminal)...
      return (si!=NULL) ? si>>m.setProb(y)>>psD : si;
    } else {
      while((si2=si>>" [")!=NULL)si=si2;
      si=si>>xs>>"] "; //cerr<<" in bracket "<<((si==NULL) ? "yes" : "no") << endl;
      while((si2=si>>" ")!=NULL)si=si2;
      si=si>>"= "; //cerr<<" in after equals "<<((si==NULL) ? "yes" : "no") << endl;

      //m.setA() = atoi(si.c_str());
      int aVar = 0;
      si=si>>aVar>>psD;
      m.setA()=aVar;
      //cerr<<" at end "<<((si==NULL) ? "yes" : "no") << endl;
      //cerr<<"  m.getA() is "<< m.getA().toInt() << endl;
      return si;
    //return (si!=NULL) ? si>>m.setA()>>psD : si;
    }
    */
  }
  ////
};

////////////////////
template <class Y,class X, class P>
bool DTree2DModel<Y,X,P>::readFields ( Array<char*>& aps ) {
  if ( /*aps[0]==sId &&*/ (3==aps.size() || 4==aps.size()) ) {
    //fprintf(stderr,"%s,%d\n",aps[3],numFields);
    assert ( '['==aps[1][0] && ']'==aps[1][strlen(aps[1])-1] );

    // Start at root...
    Tree<B,DecisNode<X,Y,P> >* ptr = this;
    assert(ptr);

    // Find appropriate node, creating nodes as necessary...
    for(int i=1; i<int(strlen(aps[1]))-1; i++) {
      char psTemp[2]="\0"; psTemp[0]=aps[1][i];
      ptr = &ptr->setBranch ( B(psTemp) );
//      assert ( '0'==aps[1][i] || '1'==aps[1][i] );
//      ptr = ( ('0'==aps[1][i]) ? &ptr->setLeft() : &ptr->setRight() ) ;
//      assert(ptr);
    }

    // Specify attribute number (at nonterminal) or probability in distribution (at terminal)...
    if      (3==aps.size()) ptr->setA()          = atoi(aps[2]);
    else if (4==aps.size()) ptr->setProb(aps[2]) = atof(aps[3]);
    else assert(false);

  } else return false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////

template<class Y, class X1,class X2, class P>
//class DTree3DModel : public Generic3DModel<Y,X1,X2,P> {
class DTree3DModel {
 private:
  // Type members...
  typedef typename X2::ElementType B;
  // Data members...
  string                               sId;
  SimpleHash<X1,DTree2DModel<Y,X2,P> > aqt;
 public:
  // Constructor / destructor methods...
  DTree3DModel ( )                 { }
  DTree3DModel ( const string& s ) { sId = s; }
  // Specification methods...
  DTree2DModel<Y,X2,P>& setTree ( const X1& x1 ) { return aqt.set(x1); }
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
bool DTree3DModel<Y,X1,X2,P>::readFields ( char* aps[], int numFields ) {
  if ( /*aps[0]==sId &&*/ (4==numFields || 5==numFields) ) {
    //fprintf(stderr,"%s,%d\n",aps[3],numFields);
    assert ( '['==aps[2][0] && ']'==aps[2][strlen(aps[2])-1] );

    // Start at root...
    Tree<B,DecisNode<X2,Y,P> >* ptr = &aqt.set(aps[1]);
    assert(ptr);

    // Find appropriate node, creating nodes as necessary...
    for(int i=1; i<strlen(aps[2])-1; i++) {
      char psTemp[2]="\0\0"; psTemp[0]=aps[1][i];
      ptr = ptr->setBranch ( B(psTemp) );
//      assert ( '0'==aps[2][i] || '1'==aps[2][i] );
//      ptr = ( ('0'==aps[2][i]) ? &ptr->setLeft() : &ptr->setRight() ) ;
//      assert(ptr);
    }

    // Specify bit (at nonterminal) or distribution (at terminal)...
    if      (4==numFields) ptr->setA()          = atoi(aps[3]);
    else if (5==numFields) ptr->setProb(aps[3]) = atof(aps[4]);
    ////    else if (5==numFields && 0==strcmp(aps[3],"0")) ptr->setProb()   = 1.0 - atof(aps[4]);
    ////    else if (5==numFields && 0==strcmp(aps[3],"1")) ptr->setProb()   = atof(aps[4]);
    else assert(false);

  } else return false;
  return true;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Trainable DTree Model
//
////////////////////////////////////////////////////////////////////////////////

template<class Y, class X, class P>
class TrainableDTree2DModel : public DTree2DModel<Y,X,P> {
 private:
  // Type members...
  typedef typename X::ElementType B;
  // Static data members...
  static List<Joint2DRV<X,Y> > lxyInitial;
 public:
  // Static member varaibles...
  static bool OUTPUT_NOISY;
  static bool OUTPUT_VERYNOISY;
  // Downcasts (safe b/c no new data)...
  TrainableDTree2DModel<Y,X,P>&       setBranch(const B& b)        { return static_cast<TrainableDTree2DModel<Y,X,P>&>       ( Tree<B,DecisNode<X,Y,P> >::setBranch(b) ); }
  const TrainableDTree2DModel<Y,X,P>& getBranch(const B& b)  const { return static_cast<const TrainableDTree2DModel<Y,X,P>&> ( Tree<B,DecisNode<X,Y,P> >::getBranch(b) ); }
  // Specification methods...
  void train ( List<Joint2DRV<X,Y> >&, const DecisNode<X,Y,P>&, const double ) ;
  void train ( const double d )                                    { train(lxyInitial,DecisNode<X,Y,P>(),d); }
  ////// Input / output methods...
  bool readData ( Array<char*>& aps ) {
    if ( 3==aps.size() ) lxyInitial.add() = Joint2DRV<X,Y> ( X(aps[1]), Y(aps[2]) );
    else if ( 4==aps.size() ) {
      for ( int i=atoi(aps[3]); i>0; i-- )
        lxyInitial.add() = Joint2DRV<X,Y> ( X(aps[1]), Y(aps[2]) );
    }
    else return false;
    return true;
  }
};
template <class Y, class X, class P> List<Joint2DRV<X,Y> > TrainableDTree2DModel<Y,X,P>::lxyInitial;
template <class Y, class X, class P> bool TrainableDTree2DModel<Y,X,P>::OUTPUT_NOISY     = false;
template <class Y, class X, class P> bool TrainableDTree2DModel<Y,X,P>::OUTPUT_VERYNOISY = false;

////////////////////
template<class Y, class X, class P>
void  TrainableDTree2DModel<Y,X,P>::train ( List<Joint2DRV<X,Y> >& lxy, const DecisNode<X,Y,P>& dnParent, const double DTREE_CHISQR_LIMIT ) {

  typedef typename DecisNode<X,Y,P>::A A;

  // Place to store counts...
  //CPT3DModel<A,B,Y,double> aaaCounts;  // hash was MUCH slower!!
  SafeArray3D<A,B,Y,double> aaaCounts ( X::getSize(), B::getDomain().getSize(), Y::getDomain().getSize(), 0.0 );
  double dTot = lxy.getCard();
  CPT1DModel<Y,double> modelY; // ( "Y_prior" );

  // For each datum in list...
  ListedObject<Joint2DRV<X,Y> >* pxy;
  for ( pxy = lxy.getFirst(); pxy; pxy = lxy.getNext(pxy) ) {
    // For each attribute position...
    for ( A a=0; a<X::getSize(); a.setNext() )
      // Add to counts...
      aaaCounts.set ( a, pxy->first.get(a.toInt()), pxy->second )++; //(pxy->second==Y("1"))?1:0 )++;
    modelY.setProb(pxy->second)++;
  }
  modelY.normalize();

//  // If best attribute's prediction is not significant...
//  else {
    // Add ratio as leaf...
    Y y;
    for ( bool by=y.setFirst(); by; by=y.setNext() )
      DecisNode<X,Y,P>::setProb(y) = (dTot>100) ? modelY.getProb(y) : (double)dnParent.getProb(y); //1.0/Y::getDomain().getSize();
    //DecisNode<Y,P>::setProb("0") = (dXX>0) ? dX0/dXX : UNIGR_FO;
    //DecisNode<Y,P>::setProb("1") = (dXX>0) ? dX1/dXX : UNIGR_FO;
    ////DTree3DModel<Y,X1,X,P>::setLeafModel().setProb(Y("0"),x1,node) = dX0/dXX;
    ////DTree3DModel<Y,X1,X,P>::setLeafModel().setProb(Y("1"),x1,node) = dX1/dXX;
//  }

  double chisqr = 0.0;
  A aBest=0;

  // Bail if will never be significant...
  if ( !lxy.isEmpty() && lxy.getCard()>1000 ) {

    // For each attribute position...
    double entBest=0.0;
    for ( A a=0; a<X::getSize(); a.setNext() ) {

      // Local model for each attrib (bit num)...
      CPT2DModel<Y,B,double> modelY_giv_B; // ( "Y_giv_B" );
      CPT1DModel<B,double>   modelB;       // ( "B_prior" );
      B b;
      for ( bool bb=b.setFirst(); bb; bb=b.setNext() ) {
        Y y;
        for ( bool by=y.setFirst(); by; by=y.setNext() ) {
          modelY_giv_B.setProb(y,b) = aaaCounts.get(a,b,y);
          modelB.setProb(b)        += aaaCounts.get(a,b,y);
        }
      }
      modelY_giv_B.normalize();
      modelB.normalize();

      // Calc entropy...
      double ent = 0.0;
      for ( bool bb=b.setFirst(); bb; bb=b.setNext() ) {
        Y y;
        for ( bool by=y.setFirst(); by; by=y.setNext() )
          ent -= (0.0==modelY_giv_B.getProb(y,b)) ? 0.0 : ( modelB.getProb(b) * modelY_giv_B.getProb(y,b) * log(modelY_giv_B.getProb(y,b)) );
      }

      // Record minimum entropy division...
      if ( a==0 || ent<entBest ) { entBest=ent; aBest=a; }
      if(OUTPUT_VERYNOISY) {
//      fprintf(stderr,"    bit=%d ent=%g (%g:%g,%g:%g) (%g:%g,%g:%g,%g:%g)\n",(int)a,ent,
//              aaaCounts.getProb(a,0,0),aaaCounts.getProb(a,0,1),aaaCounts.getProb(a,1,0),aaaCounts.getProb(a,1,1),
//              modelB.getProb(0),modelB.getProb(1),
//              modelY_giv_B.getProb(0,0),modelY_giv_B.getProb(1,0),modelY_giv_B.getProb(0,1),modelY_giv_B.getProb(1,1));
//              //d0X,d1X,d00giv0X,d01giv0X,d10giv1X,d11giv1X);
        fprintf(stderr,"    bit=%d ent=%g\n",a.toInt(),ent);
        Y y;
        for ( bool by=y.setFirst(); by; by=y.setNext() )
          fprintf(stderr,"     \t%s\t%s%.1f %s%.1f %s%.1f %s%.1f %s%.1f %s%.1f ...\n",y.getString().c_str(),
                  B(0).getString().c_str(),modelY_giv_B.getProb(y,0),
                  B(1).getString().c_str(),modelY_giv_B.getProb(y,1),
                  B(2).getString().c_str(),modelY_giv_B.getProb(y,2),
                  B(3).getString().c_str(),modelY_giv_B.getProb(y,3),
                  B(4).getString().c_str(),modelY_giv_B.getProb(y,4),
                  B(5).getString().c_str(),modelY_giv_B.getProb(y,5));
          //fprintf(stderr,"      %s %g:%g\n",y.getString().c_str(),modelY_giv_B.getProb(y,0),modelY_giv_B.getProb(y,1));
      }
    }

    if(OUTPUT_NOISY)
      fprintf(stderr,"  tot=%08d bestbit=%d bestent=%g\n",(int)dTot,aBest.toInt(),entBest);

    chisqr = 0.0;
    if ( X::getSize()>0 ) {
      // Local model for each attrib (bit num)...
      CPT1DModel<B,double> modelB_giv_Abest; // ( "B_giv_Abest" );
      B b;
      for ( bool bb=b.setFirst(); bb; bb=b.setNext() ) {
        Y y;
        for ( bool by=y.setFirst(); by; by=y.setNext() ) {
          modelB_giv_Abest.setProb(b) += aaaCounts.get(aBest,b,y);
        }
      }
      modelB_giv_Abest.normalize();

      // Calc chi sqr...
      for ( bool bb=b.setFirst(); bb; bb=b.setNext() ) {
        Y y;
        for ( bool by=y.setFirst(); by; by=y.setNext() ) {
          if ( modelB_giv_Abest.getProb(b)>0 && modelY.getProb(y)>0 && dTot>0 ) {
            double exp = modelB_giv_Abest.getProb(b) * modelY.getProb(y) * dTot;
            chisqr += pow ( aaaCounts.get(aBest,b,y)-exp, 2 ) / exp;
          }
        }
      }
    }

    if(OUTPUT_NOISY)
      fprintf(stderr,"  chisqr=%g\n",chisqr);
  }

  // If best bit prediction is significant...
  if ( chisqr > DTREE_CHISQR_LIMIT ) {
    //// // Add node to model fields...
    //// DTree3DModel<Y,X1,X,P>::setNodeMap()[Joint2DRV<X1,NodeNum>(x1,node)] = aBest;
    // Split list into each 0/1 child of this node...
    SimpleHash<B,List<Joint2DRV<X,Y> > > alxy;
    // For each datum in list...
    while ( !lxy.isEmpty() ) {
      Joint2DRV<X,Y>* pxy = lxy.getFirst();
      alxy[pxy->first.get(aBest.toInt())].add() = *pxy;
      lxy.pop();
    }
    // Recursively call train at each child...
    DecisNode<X,Y,P>::setA()=aBest;
    B b;
    for ( bool bb=b.setFirst(); bb; bb=b.setNext() )
      setBranch(b).train ( alxy[b], *this, DTREE_CHISQR_LIMIT );
    // setLeft().train  ( alxy[0], DTREE_CHISQR_LIMIT );  ////node*2LL);
    // setRight().train ( alxy[1], DTREE_CHISQR_LIMIT );  ////node*2LL+1LL);
  }
}


////////////////////////////////////////////////////////////////////////////////

template<class Y, class X1, class X2, class P>
class TrainableDTree3DModel : public DTree3DModel<Y,X1,X2,P> {

 private:

  map<X1,List<Joint2DRV<X2,Y> > > mqlxy;

 public:

  // Static member varaibles...
  static bool OUTPUT_NOISY;

  ////// Constructor...
  TrainableDTree3DModel(const char* ps) : DTree3DModel<Y,X1,X2,P>(ps) { }

  ////// setTree downcast...
  TrainableDTree2DModel<Y,X2,P>& setTree(const X1& x1) { return static_cast<TrainableDTree2DModel<Y,X2,P>&>(DTree3DModel<Y,X1,X2,P>::setTree(x1)); }

  ////// Add training data to per-subphone lists...
  bool readData ( Array<char*>& aps ) {
    if ( 4==aps.size() ) {
      mqlxy[X1(aps[1])].add() = Joint2DRV<X2,Y> ( X2(aps[2]), Y(aps[3]) );
      ////mqlxy[X1(aps[1])].getLast()->write(stderr); fprintf(stderr,"\n");
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

template <class Y, class X1, class X2, class P> bool TrainableDTree3DModel<Y,X1,X2,P>::OUTPUT_NOISY = false;


#endif // _NL_DTREE__
