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


////////////////////////////////////////////////////////////////////////////////

template<class X>
class QuadConvolved : public X {
 public:
  typedef typename X::ElementType ElementType;
  static const unsigned int SIZE = X::SIZE+1;
 private:
  ElementType eC;
 public:
  QuadConvolved<X> ( ) : eC(1.0) { }
  ElementType& operator[] ( unsigned int i )       { assert(i<SIZE); return (i<SIZE-1) ? X::operator[](i) : eC; }
  ElementType  operator[] ( unsigned int i ) const { assert(i<SIZE); return (i<SIZE-1) ? X::operator[](i) : eC; }
  ElementType& set        ( unsigned int i )       { assert(i<SIZE); return (i<SIZE-1) ? X::set(i) : eC; }
  ElementType  get        ( unsigned int i ) const { assert(i<SIZE); return (i<SIZE-1) ? X::get(i) : eC; }
  friend pair<StringInput,QuadConvolved<X>*> operator>> ( StringInput si, QuadConvolved<X>& x ) { return pair<StringInput,QuadConvolved<X>*>(si,&x); }
  friend StringInput operator>> ( pair<StringInput,QuadConvolved<X>*> si_x, const char* psD ) {
    StringInput si = si_x.first>>static_cast<X&>(*si_x.second)>>psD;
    si_x.second->set(SIZE-1)=0.0;
    for(int i=0;i<SIZE-1;i++) si_x.second->set(SIZE-1)+=pow(si_x.second->get(i)/1000000.0,2.0);
    return si; }
  friend ostream& operator<< ( ostream& os, const QuadConvolved<X>& x ) { os<<x.get(0); for(int i=1;i<SIZE;i++)os<<","<<x.get(i); return os; }
};


////////////////////////////////////////////////////////////////////////////////

template<class X>
class Shifted : public X {
 public:
  typedef typename X::ElementType ElementType;
  static const unsigned int SIZE = X::SIZE+1;
 private:
  ElementType e0;
 public:
  Shifted<X> ( ) : e0(0.0) { }
  ElementType& operator[] ( unsigned int i )       { assert(i<SIZE); return (i<SIZE-1) ? X::operator[](i) : e0; }
  ElementType  operator[] ( unsigned int i ) const { assert(i<SIZE); return (i<SIZE-1) ? X::operator[](i) : e0; }
  ElementType& set        ( unsigned int i )       { assert(i<SIZE); return (i<SIZE-1) ? X::set(i) : e0; }
  ElementType  get        ( unsigned int i ) const { assert(i<SIZE); return (i<SIZE-1) ? X::get(i) : e0; }
  friend pair<StringInput,Shifted<X>*> operator>> ( StringInput si, Shifted<X>& x ) { return pair<StringInput,Shifted<X>*>(si,&x); }
  //friend StringInput operator>> ( pair<StringInput,Shifted<X>*> si_x, const char* psD ) {
  //  StringInput si=si_x.first; for(int i=0;i<SIZE;i++) si=si>>si_x.second->set(i)>>((i<SIZE-1)?",":psD); return si; }
  friend StringInput operator>> ( pair<StringInput,Shifted<X>*> si_x, const char* psD ) {
    StringInput si = si_x.first>>static_cast<X&>(*si_x.second)>>psD; si_x.second->set(SIZE-1)=1.0; return si; }
  //friend ostream& operator<< ( ostream& os, const Shifted<X>& x ) { os<<x.get(0); for(uint i=1;i<SIZE;i++)os<<","<<x.get(i); return os; }
  friend ostream& operator<< ( ostream& os, Shifted<X>& x ) { if(x.get(SIZE-1)!=1.0)for(uint i=1;i<SIZE;i++)x.set(i)/=x.get(SIZE-1); return os<<((X)x); }
};


////////////////////////////////////////////////////////////////////////////////

template<class X>
class Vector : public X {
 public:
  typedef typename X::ElementType ElementType;
  static const unsigned int SIZE = X::SIZE;
  Vector ( ) { }
  Vector ( const Vector<X>& v ) { for(uint i=0;i<SIZE;i++) X::set(i)=v.get(i); }
  ElementType      operator* ( const Vector<X>& v ) const          { ElementType d=0.0; for(uint i=0;i<SIZE;i++) d += X::get(i)*v.get(i); return d; } // inner prod
  Vector<X>        operator+ ( const Vector<X>& v ) const          { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i]=X::get(i)+v[i]; return vO; }   // vector sum
  Vector<X>        operator- ( const Vector<X>& v ) const          { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i]=X::get(i)-v[i]; return vO; }   // vector sum
  Vector<X>        operator* ( ElementType d ) const               { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i] = X::get(i)*d; return vO; }
  Vector<X>        operator/ ( ElementType d ) const               { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i] = X::get(i)/d; return vO; }
  Vector<X>        operator+ ( ElementType d ) const               { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i] = X::get(i)+d; return vO; }
  Vector<X>        operator- ( ElementType d ) const               { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i] = X::get(i)-d; return vO; }
  friend Vector<X> operator* ( ElementType d, const Vector<X>& v ) { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i] = d*v[i]; return vO; }
  friend Vector<X> operator/ ( ElementType d, const Vector<X>& v ) { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i] = d/v[i]; return vO; }
  friend Vector<X> operator+ ( ElementType d, const Vector<X>& v ) { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i] = d+v[i]; return vO; }    
  friend Vector<X> operator- ( ElementType d, const Vector<X>& v ) { Vector<X> vO;      for(uint i=0;i<SIZE;i++) vO[i] = d-v[i]; return vO; }    
  Vector<X>&       operator*= ( ElementType d )                    { for(uint i=0;i<SIZE;i++) X::set(i)*=d; return *this; }
  Vector<X>&       operator/= ( ElementType d )                    { for(uint i=0;i<SIZE;i++) X::set(i)/=d; return *this; }
  Vector<X>&       operator+= ( ElementType d )                    { for(uint i=0;i<SIZE;i++) X::set(i)+=d; return *this; }
  Vector<X>&       operator-= ( ElementType d )                    { for(uint i=0;i<SIZE;i++) X::set(i)-=d; return *this; }
  Vector<X>&       operator+= ( const Vector<X>& v )               { for(uint i=0;i<SIZE;i++) X::set(i)+=v.X::get(i); return *this; }
  Vector<X>&       operator-= ( const Vector<X>& v )               { for(uint i=0;i<SIZE;i++) X::set(i)-=v.X::get(i); return *this; }
};


////////////////////////////////////////////////////////////////////////////////

//#define GD 1

template<class Q,class X,class P>
class LinSepModel {
 public:
  class CondVarType : public Vector<Shifted<X> > { public: CondVarType(){} CondVarType(const Vector<Shifted<X> >& v):Vector<Shifted<X> >(v){} };
  class InputTrainingExample : public Joint2DRV<Q,CondVarType> {
//   public:
//    friend pair<StringInput,InputTrainingExample*> operator>> ( StringInput si, InputTrainingExample& x ) { return pair<StringInput,InputTrainingExample*>(si,&x); }
//    friend StringInput operator>> ( pair<StringInput,InputTrainingExample*> si_x, const char* psD ) {
//      StringInput si = si_x.first>>static_cast<X&>(*si_x.second)>>psD; si_x.second->set(SIZE-1)=1.0; return si; }
  };
  typedef P ProbType;
  CondVarType w;            // weight vector

 private:

  typename X::ElementType sigmoid ( double a ) { return (!isnan(1.0/(1.0+exp(-a)))) ? 1.0/(1.0+exp(-a)) : (a>0.0) ? 1.0 : 0.0; }

 public:

  template<class F>
  void train ( const SubArray<SafePtr<const InputTrainingExample> > apyx, F& yfn, double iP, double iN ) {
    const double SCALER = 1000.0; // 1000.0 works just as well
    //cerr<<"      ---"<<iN<<" "<<iP<<" "<<N<<"\n";
    ////cout<<"      ---"<<iN<<" "<<iP<<" "<<N<<"\n";
    iN*=2.0/apyx.size(); iP*=2.0/apyx.size();
    cerr<<"      ---"<<iN<<" "<<iP<<"\n";

    #ifdef GD
    #else
    //typename X::ElementType lambda=1.0;             // inverse variance of prior over total weight
    typename X::ElementType beta;                   // hestenes steifel factor
    //typename X::ElementType ddl;                    // double deriv (for newton step)
    #endif
    CondVarType gPrev;                              // previous gradient
    CondVarType g;                                  // current gradient
    CondVarType u;                                  // step direction
    typename X::ElementType z=0.0;                  // step size
    double errPrev=200000.0;
    double err    =100000.0;
    Array<typename X::ElementType> wtx(apyx.size(),0.0); // weighted sum

    for ( int k=0; k<100 && err>0.0 && (errPrev-err>0.001||k<=3); k++ ) {
      errPrev = err;

      ////cerr<<" ? "<<g<<"\n";
      g = CondVarType(); //-lambda*w
      ////cerr<<" ??? "<<iP<<" "<<iN<<"\n";
      for ( unsigned int n=0; n<apyx.size(); n++ ) {
        g += ( sigmoid ( -((yfn(apyx[n].getRef().first))?1.0:-1.0) * wtx[n] )
               * ((yfn(apyx[n].getRef().first))?1.0:-1.0) * apyx[n].getRef().second/SCALER
               * ((yfn(apyx[n].getRef().first))?iN:iP) );
      }
      //cerr<<" g="<<g<<"\n";

      #ifdef GD
      u = g;
      #else
      beta = (k<=1) ? 0.0 : (g*(g-gPrev)) / (u*(g-gPrev));
      //cerr<<" beta="<<beta<<"\n";
      u = gPrev - beta*u;
      #endif

      #ifdef GD
      #else
      //ddl = 0.0; //(lambda*u*u);
      //for ( unsigned int n=0; n<apyx.size(); n++ )
      //  ddl += sigmoid(wtx[n]) * sigmoid(-wtx[n]) * pow(u*apyx[n].getRef().second,2);
      //z = (g*u)/ddl;
      #endif

      //if(apyx.size()==123) cerr<<" u="<<u<<"\n";

      if (k<=0) {
        //w=u;
        errPrev=100000.0;
      } else {
        double zDiff=10.0;
        z=0.0;
        for ( int t=0; abs(zDiff)>.001 && t<1; t++ ) {
          double a = 0.0;
          double b = 0.0;
          ////cerr<<"      w = "<<w<<"\n";
          for ( unsigned int n=0; n<apyx.size(); n++ ) {
            ////cout<<"          !!: "<<apyx[n].getRef().second<<"\n";

            // If only one z loop, use wtx...
            double ywzux = ((yfn(apyx[n].getRef().first))?1.0:-1.0) * wtx[n];
            //double ywzux = ((yfn(apyx[n].getRef().first))?1.0:-1.0) * (w+z*u) * apyx[n].getRef().second/SCALER;

            //if(apyx.size()==123) cerr<<" w="<<w<<"\n";
            //if(apyx.size()==123) cerr<<" z="<<z<<"\n";
            //if(apyx.size()==123) cerr<<" u="<<u<<"\n";
            //if(apyx.size()==123) cerr<<" z*u="<<z*u<<"\n";
            //if(apyx.size()==123) cerr<<" (w+z*u)="<<(w+(z*u))<<"\n";
            //if(apyx.size()==123) cerr<<" x="<<(apyx[n].getRef().second/SCALER)<<"\n";
            a += sigmoid(-ywzux) * ((yfn(apyx[n].getRef().first))?1.0:-1.0) * u * apyx[n].getRef().second/SCALER * ((yfn(apyx[n].getRef().first))?iN:iP);
            b += sigmoid(ywzux) * sigmoid(-ywzux) * pow(u*apyx[n].getRef().second/SCALER,2.0)                    * ((yfn(apyx[n].getRef().first))?iN:iP);
            //if(apyx.size()==123) cerr<<"  x_"<<n<<"="<<ywzux<<" a="<<a<<" b="<<b<<"\n";
          }
          zDiff = (a==0.0) ? 0.0 : a/b;
          cerr<<"          (z="<<z<<")";
          z += zDiff;
          cerr<<" a="<<a<<" b="<<b<<" zDiff="<<zDiff<<"  ==>  z="<<z<<"\n";
        }
        w += z*u;
      }

      //cerr<<" w="<<w<<"\n";
      for ( unsigned int n=0; n<apyx.size(); n++ )
        wtx[n] += z*u*apyx[n].getRef().second/SCALER;
      gPrev = g;

      err = 0.0;
      for ( unsigned int n=0; n<apyx.size(); n++ )
        err += log ( 1.0 + exp ( -((yfn(apyx[n].getRef().first))?1.0:-1.0) * w*apyx[n].getRef().second/SCALER ) ) * ((yfn(apyx[n].getRef().first))?iN:iP);
      cerr<<"       ? k="<<k<<" "<<err<<"\n";
    }

    //for ( unsigned int n=0; n<apyx.size(); n++ )
    //  if ( yfn(apyx[n].getRef().first) ) cerr<<"          "<<( ((yfn(apyx[n].getRef().first))?1.0:-1.0) * w*apyx[n].getRef().second )<<"\n";
  }
  bool classify ( const CondVarType& x ) const { /*cerr<<"----------\n"<<w<<"\n *\n"<<x<<"\n =\n"<<(w*x)<<"\n";*/ return ( w*x > 0.0 ); }
  friend ostream& operator<< ( ostream& os, const LinSepModel<Q,X,P>& m ) { return os<<m.w; }
  friend pair<StringInput,LinSepModel<Q,X,P>*> operator>> ( StringInput si, LinSepModel<Q,X,P>& m ) {
    return pair<StringInput,LinSepModel<Q,X,P>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,LinSepModel<Q,X,P>*> si_m, const char* psD ) {
    return (si_m.first==NULL) ? StringInput(NULL) : si_m.first>>si_m.second->w>>psD; }
};


////////////////////////////////////////////////////////////////////////////////

template<class Q,class X,class P>
  class SimpleLinSepModel {
 public:
  class CondVarType : public Vector<X> { };
  class InputTrainingExample : public Joint2DRV<Q,CondVarType> {
//   public:
//    friend pair<StringInput,InputTrainingExample*> operator>> ( StringInput si, InputTrainingExample& x ) { return pair<StringInput,InputTrainingExample*>(si,&x); }
//    friend StringInput operator>> ( pair<StringInput,InputTrainingExample*> si_x, const char* psD ) {
//      StringInput si = si_x.first>>static_cast<X&>(*si_x.second)>>psD; si_x.second->set(SIZE-1)=1.0; return si; }
  };
  typedef P ProbType;
  CondVarType w;            // weight vector

 private:

  typename X::ElementType sigmoid ( double a ) { return 1.0/(1.0+exp(-a)); }

 public:

  ////void train ( const SafePtr<const InputTrainingExample> apyx[], const int N, bool fn(const Q&), double iPnotused, double iNnotused ) {
  template<class F>
  void train ( const SubArray<SafePtr<const InputTrainingExample> > apyx, F& yfn, double iPnotused, double iNnotused ) {
    double dIntercept = 0.0;
    CondVarType vP,vN,vDelt,vMid;  // pos/neg centroids, difference, midpoint
    int iP=0,iN=0;       // pos/neg counts
    // For each example...
    for ( unsigned int i=0; i<apyx.size(); i++ ) {
      // If matches target, avg into pos centroid...
      if ( yfn(apyx[i].getRef().first) ) {
        iP++;
        for ( unsigned int j=0; j<CondVarType::SIZE; j++ ) vP[j]+=apyx[i].getRef().second[j];
      }
      // If no match, avg into neg centroid...
      else {
        iN++;
        for ( unsigned int j=0; j<CondVarType::SIZE; j++ ) vN[j]+=apyx[i].getRef().second[j];
      }
    }
    //cerr<<"prob of "<<qTarget<<" at "<<branch<<" = "<<iP<<"/"<<N<<":"<<iN<<"/"<<N<<" = "<<double(iP)/double(N)<<"\n";
    // Turn sums into avgs...
    if ( iP>0 ) for ( unsigned int j=0; j<CondVarType::SIZE; j++ ) vP[j]/=double(iP);
    if ( iN>0 ) for ( unsigned int j=0; j<CondVarType::SIZE; j++ ) vN[j]/=double(iN);

    // Calc vec of delta between centroids...
    for ( unsigned int j=0; j<CondVarType::SIZE; j++ ) vDelt[j]=vP[j]-vN[j];
    // Calc vec of midpoint between centroids...
    for ( unsigned int j=0; j<CondVarType::SIZE; j++ ) vMid[j]=(vP[j]+vN[j])/2.0;
    // Calc y-intercept of gradient (negative so test is > -1.0)...
    for ( unsigned int j=0; j<CondVarType::SIZE; j++ ) dIntercept-=vDelt[j]*vMid[j];
    // Calc gradient between centroids...
    for ( unsigned int j=0; j<CondVarType::SIZE; j++ ) w[j]=vDelt[j]/dIntercept;
  }
  // NOTE: bool does not necessarily correspond to true=positive -- have to check by hand if you care (oblidtree doesn't care)...
  bool classify ( const CondVarType& x ) const { /*cerr<<"----------\n"<<w<<"\n *\n"<<x<<"\n =\n"<<(w*x)<<"\n";*/ return ( w*x > -1.0 ); }

  friend ostream& operator<< ( ostream& os, const SimpleLinSepModel<Q,X,P>& m ) { return os<<m.w; }

  friend pair<StringInput,SimpleLinSepModel<Q,X,P>*> operator>> ( StringInput si, SimpleLinSepModel<Q,X,P>& m ) {
    return pair<StringInput,SimpleLinSepModel<Q,X,P>*>(si,&m); }
  friend StringInput operator>> ( pair<StringInput,SimpleLinSepModel<Q,X,P>*> si_m, const char* psD ) {
    return (si_m.first==NULL) ? StringInput(NULL) : si_m.first>>si_m.second->w>>psD; }
};
