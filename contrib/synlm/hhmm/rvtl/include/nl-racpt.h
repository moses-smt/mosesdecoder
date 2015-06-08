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


#ifndef _NL_RACPT__
#define _NL_RACPT__


template<class K, class P>
class GenericRACPTModel : public SimpleHash<K,P> {
 private:
  typedef SimpleHash<K,P> HKP;
//  typedef typename SimpleHash<Y,P>::const_iterator IYP;
  //HKYP h;

 public:
  //typedef Y RVType;
  //typedef BaseIterVal<std::pair<IYP,IYP>,Y> IterVal;
  //typedef typename HKYP::const_iterator const_key_iterator;

  bool contains ( const K& k ) const {
    return ( SimpleHash<K,P>::contains(k) );
  }

/*
  P getProb ( const IterVal& ikyp, const K& k ) const {
    if ( ikyp.iter.first == ikyp.iter.second ) { cerr<<"ERROR: no iterator to fix probability: "<<k<<endl; return P(); }
    return ( ikyp.iter.first->second );
  }
*/

  P getProb ( const K& k ) const {
    return SimpleHash<K,P>::get(k);
  }
  P& setProb ( const K& k ) {
    return SimpleHash<K,P>::set(k);
  }

/*
  void normalize ( ) {
    for ( typename HKYP::const_iterator ik=HKYP::begin(); ik!=HKYP::end(); ik++ ) {
      K k=ik->first;
      P p=P();
      IterVal y;
      for(bool by=setFirst(y,k); by; by=setNext(y,k))
        p+=getProb(y,k);
      if (p!=P())
        for(bool by=setFirst(y,k); by; by=setNext(y,k))
          setProb(y,k)/=p;
    }
  }
*/
/*
   void transmit ( int tSockfd, const char* psId ) const {
    for ( typename HKYP::const_iterator ik=HKYP::begin(); ik!=HKYP::end(); ik++ ) {
      K k=ik->first;
      IterVal y;
      // For each non-zero probability in model...
      for ( bool b=setFirst(y,k); b; b=setNext(y,k) ) {
        //if ( getProb(y,k) != P() ) {
          String str(1000);
          str<<psId<<" "<<k<<" : "<<y<<" = "<<getProb(y,k).toDouble()<<"\n";
          if ( send(tSockfd,str.c_array(),str.size()-1,MSG_EOR) != int(str.size()-1) )
            {cerr<<"ERROR writing to socket\n";exit(0);}
        //}
      }
    }
  }
*/
  void dump ( ostream& os, const char* psId ) const {
    for ( typename HKP::const_iterator ik=HKP::begin(); ik!=HKP::end(); ik++ ) {
      K k=ik->first;
      os << psId<<" "<<k<<" = "<<getProb(k).toDouble()<<endl;

//      IterVal y;
//      for ( bool b=setFirst(y,k); b; b=setNext(y,k) )
//        os<<psId<<" "<<k<<" : "<<y<<" = "<<getProb(y,k).toDouble()<<"\n";
    }
  }
  void subsume ( GenericRACPTModel<K,P>& m ) {
    for ( typename HKP::const_iterator ik=m.HKP::begin(); ik!=m.HKP::end(); ik++ ) {
      K k=ik->first;
      setProb(k) = m.getProb(k);
//      IterVal y;
//      for ( bool b=m.setFirst(y,k); b; b=m.setNext(y,k) )
//        setProb(y,k) = m.getProb(y,k);
    }
  }
  void clear ( ) { SimpleHash<K,P>::clear(); }

  friend pair<StringInput,GenericRACPTModel<K,P>*> operator>> ( StringInput si, GenericRACPTModel<K,P>& m ) {
    return pair<StringInput,GenericRACPTModel<K,P>*>(si,&m); }

  friend StringInput operator>> ( pair<StringInput,GenericRACPTModel<K,P>*> delimbuff, const char* psD ) {
    K k;
    StringInput si,si2,si3;
    GenericRACPTModel<K,P>& m = *delimbuff.second;
    si=delimbuff.first;
    if ( si==NULL ) return si;

    // Kill the colon since we're treating the whole thing as the condition
    char * str = si.c_str();
    char * p = strchr(str, ':');
    if(p){
      p[0] = ' ';
    }
    si=str;
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>k>>" ";
    while((si2=si>>" ")!=NULL)si=si2;
    si=si>>"= ";
    while((si2=si>>" ")!=NULL)si=si2;
    return (si!=NULL) ? si>>m.setProb(k)>>psD : si;
  }
};


template<class Y, class P>
class RandAccCPT1DModel : public GenericRACPTModel<MapKey1D<Y>,P> {
 public:
//  typedef typename GenericCPTModel<Y,MapKey1D<Unit>,P>::IterVal IterVal;

  bool contains ( const Y& y ) const {
    return GenericRACPTModel<MapKey1D<Y>,P>::contains ( MapKey1D<Y>(y) );
  }
/*
  P getProb ( const IterVal& ixyp ) const {
    return GenericCPTModel<MapKey1D<Y>,P>::getProb ( ixyp, MapKey1D<Y>(Y()) );
  }
*/
  P getProb ( const Y& y ) const {
    return GenericRACPTModel<MapKey1D<Y>,P>::getProb ( MapKey1D<Y>(y) );
  }
/*
P& setProb ( const Y& y ) {
    cerr << "setProb called on racpt1d" << endl;
    return GenericRACPTModel<MapKey1D<Y>,P>::setProb ( MapKey1D<Y>(y) );
  }
*/
  /*
  bool readFields ( Array<char*>& aps ) {
    if ( 3==aps.size() ) {
      GenericRACPTModel<MapKey1D<Y>,P>::setProb ( MapKey1D<Y>(aps[1]) ) = atof(aps[2]);
      return true;
    }
    return false;
  }
  */
};


////////////////////
template<class Y, class X1, class P>
class RandAccCPT2DModel : public GenericRACPTModel<MapKey2D<X1,Y>,P> {
 public:

  // This stuff only for deterministic 'Determ' models...
//  typedef X1 Dep1Type;
//  typedef P ProbType;
//  MapKey1D<X1> condKey;

  bool contains ( const Y& y, const X1& x1 ) const {
//    MapKey2D<X1,Y> temp = MapKey2D<X1,Y>(x1,y);
    return GenericRACPTModel<MapKey2D<X1,Y>,P>::contains ( MapKey2D<X1,Y>(x1,y) );
  }

  P getProb ( const Y& y, const X1& x1 ) const {
    return GenericRACPTModel<MapKey2D<X1,Y>,P>::getProb ( MapKey2D<X1,Y>(x1,y) );
  }

/*
  P& setProb ( const Y& y, const X1& x1 ) {
    cerr << "setProb called on racpt2d" << endl;
    return GenericRACPTModel<MapKey2D<Y,X1>,P>::setProb ( MapKey2D<Y,X1>(y,x1) );
  }
*/

};


////////////////////
template<class Y, class X1, class X2, class P>
class RandAccCPT3DModel : public GenericRACPTModel<MapKey3D<X1,X2,Y>,P> {
 public:

  bool contains ( const Y& y, const X1& x1, const X2& x2 ) const {
    return GenericRACPTModel<MapKey3D<X1,X2,Y>,P>::contains ( MapKey3D<X1,X2,Y>(x1,x2,y) );
  }

  P getProb ( const Y& y, const X1& x1, const X2& x2 ) const {
    return GenericRACPTModel<MapKey3D<X1,X2,Y>,P>::getProb ( MapKey3D<X1,X2,Y>(x1,x2,y) );
  }
/*
  P& setProb ( const Y& y, const X1& x1, const X2& x2 ) {
    return GenericRACPTModel<MapKey3D<X1,X2,Y>,P>::setProb (  MapKey3D<Y,X1,X2>(x1,x2,y) );
  }
*/
};

/*
////////////////////
template<class Y, class X1, class X2, class X3, class P>
class CPT4DModel : public GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P> {
 public:
  typedef typename GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::IterVal IterVal;

  bool setFirst ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::setFirst ( ixyp, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  bool setNext ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::setNext ( ixyp, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  bool contains ( const Y& y, const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::contains ( y, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::contains ( MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  P getProb ( const IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::getProb ( ixyp, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  P getProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3 ) const {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::getProb ( y, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3 ) {
    return GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::setProb ( y, MapKey3D<X1,X2,X3>(x1,x2,x3) );
  }
  bool readFields ( Array<char*>& aps ) {
    if ( 6==aps.size() ) {
      GenericCPTModel<Y,MapKey3D<X1,X2,X3>,P>::setProb ( Y(aps[4]), MapKey3D<X1,X2,X3>(aps[1],aps[2],aps[3]) ) = atof(aps[5]);
      return true;
    }
    return false;
  }
};


////////////////////
template<class Y, class X1, class X2, class X3, class X4, class P>
class CPT5DModel : public GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P> {
 public:
  typedef typename GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::IterVal IterVal;

  bool setFirst ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::setFirst ( ixyp, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  bool setNext ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::setNext ( ixyp, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  bool contains ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::contains ( y, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::contains ( MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  P getProb ( const IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::getProb ( ixyp, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  P getProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) const {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::getProb ( y, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4 ) {
    return GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::setProb ( y, MapKey4D<X1,X2,X3,X4>(x1,x2,x3,x4) );
  }
  bool readFields ( Array<char*>& aps ) {
    if ( 7==aps.size() ) {
      GenericCPTModel<Y,MapKey4D<X1,X2,X3,X4>,P>::setProb ( Y(aps[5]), MapKey4D<X1,X2,X3,X4>(aps[1],aps[2],aps[3],aps[4]) ) = atof(aps[6]);
      return true;
    }
    return false;
  }
};


////////////////////
template<class Y, class X1, class X2, class X3, class X4, class X5, class P>
class RACPT6DModel : public GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P> {
 public:
  typedef typename GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::IterVal IterVal;

  bool setFirst ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::setFirst ( ixyp, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  bool setNext ( IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::setNext ( ixyp, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  bool contains ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::contains ( y, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  bool contains ( const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::contains ( MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  P getProb ( const IterVal& ixyp, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::getProb ( ixyp, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  P getProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) const {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::getProb ( y, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  P& setProb ( const Y& y, const X1& x1, const X2& x2, const X3& x3, const X4& x4, const X5& x5 ) {
    return GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::setProb ( y, MapKey5D<X1,X2,X3,X4,X5>(x1,x2,x3,x4,x5) );
  }
  bool readFields ( Array<char*>& aps ) {
    if ( 8==aps.size() ) {
      GenericCPTModel<Y,MapKey5D<X1,X2,X3,X4,X5>,P>::setProb ( Y(aps[6]), MapKey5D<X1,X2,X3,X4,X5>(aps[1],aps[2],aps[3],aps[4],aps[5]) ) = atof(aps[7]);
      return true;
    }
    return false;
  }
};

*/
#endif //_NL_RACPT__
