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

#ifndef _NL_DENOT__
#define _NL_DENOT__

#include "nl-safeids.h"
#include "nl-prob.h"
#include "nl-const.h"
#include "nl-randvar.h"
#include "nl-cpt.h"
#include <string>

////////////////////////////////////////////////////////////////////////////////
//
//  VecE
//
////////////////////////////////////////////////////////////////////////////////

template<int N,class I,class RC=ReaderContext>
class VecE : public StaticSafeArray<N,I> {        // Coindexation vector
 public:
  typedef I  IntType;
  typedef RC RCType;
  static const int NUM_ENTS;
  // Constructor / destructor methods...
  VecE ( )                         : StaticSafeArray<N,I>(-1) { }
  VecE ( const string& s, RC& rc ) : StaticSafeArray<N,I>(-1) { read(s,rc); }
  // Specification methods...
  void unOffset    ( const ReaderContext& rc ) {  }
  void pushOffsets ( const ReaderContext& rc ) {  }
  // Extraction methods...
  //int  getHashConst ( const int m ) const { int k=1; for(int i=0;i<NUM_ENTS;i++){k=(k<<sizeof(I))%m; } return k; }
  //int  getHashKey   ( const int m ) const { int k=0; for(int i=0;i<NUM_ENTS;i++){k=k<<sizeof(I); k+=abs(StaticSafeArray<N,I>::get(i)); k%=m; } return k; }
  size_t getHashKey   ( ) const { size_t k=0; for(int i=0;i<NUM_ENTS;i++){k=rotLeft(k,3); k^=StaticSafeArray<N,I>::get(i).toInt(); } return k; }
  bool operator< (const VecE<N,I,RC>& e) const { return StaticSafeArray<N,I>::operator<(e);  }
  bool operator==(const VecE<N,I,RC>& e) const { return StaticSafeArray<N,I>::operator==(e); }
  void readercontextPush ( const ReaderContext& rc ) const { }
  void readercontextPop  ( const ReaderContext& rc ) const { }
  // Input / output methods...
  void read  ( char* ps, const ReaderContext& ) ;
  void write ( FILE*, ReaderContext& ) const ;
  void write ( FILE* pf ) const { ReaderContext rc; write(pf,rc); }
  string getString() const { ReaderContext rc; return getString(rc); }
  string getString( ReaderContext& ) const;
};
template<int N,class I,class RC> const int VecE<N,I,RC>::NUM_ENTS = N;

////////////////////
template<int N,class I, class RC>
void VecE<N,I,RC>::read ( char* ps, const ReaderContext& rc ) {
  /*
  ////fprintf(stderr,"VecE::VecE in\n");
  int i;
  string::size_type j;
  // Chop into individual coinds strings...
  for ( i=0, j=0;  s!="" && s!="," && j!=string::npos;  i++, j=s.find_first_of(',',j), j=(-1==j)?j:j+1 )
    StaticSafeArray<N,I>::set(i) = s.substr ( j, s.find_first_of(',',j)-j );
  ////fprintf(stderr,"VecE::VecE out\n");
  */
  char* psT; int i=0;
  for ( char* psU=strtok_r(ps,",",&psT);
        psU && i<NUM_ENTS;
        psU=strtok_r(NULL,",",&psT),i++ )
    StaticSafeArray<N,I>::set(i) = psU;
}

////////////////////
template<int N,class I, class RC>
void VecE<N,I,RC>::write ( FILE* pf, ReaderContext& rc ) const {
  for(int i=0; i<NUM_ENTS; i++)
    if(StaticSafeArray<N,I>::get(i) >= 0 ) {
      if(i) fprintf(pf,","); StaticSafeArray<N,I>::get(i).write(pf);
    }
}

template<int N, class I, class RC>
string VecE<N,I,RC>::getString( ReaderContext& rc ) const {
   string rString;
   for(int i=0; i<NUM_ENTS; i++)
     if(StaticSafeArray<N,I>::get(i) >= 0 ) {
       if(i) rString += ",";
       rString += StaticSafeArray<N,I>::get(i).getString();
     }
   return rString;
}

////////////////////////////////////////////////////////////////////////////////
//
//  VecVReaderContext
//
////////////////////////////////////////////////////////////////////////////////

class VecVReaderContext : public ReaderContext {
 public:
  map<string,int> msi;
  int             offset;
  VecVReaderContext ( ) { offset=0; }
};

////////////////////////////////////////////////////////////////////////////////
//
//  VecV
//
////////////////////////////////////////////////////////////////////////////////

template<int N,class I,class RC=VecVReaderContext,int ND1=0,int ND2=0>
class VecV : public StaticSafeArray<N,I> {        // Coindexation vector
 public:
  typedef I  IntType;
  typedef RC RCType;
  static const int NUM_ENTS;
  static const int NUM_ENTS_DEP1;
  static const int NUM_ENTS_DEP2;
  // Constructor / destructor methods...
  VecV ( )                                        : StaticSafeArray<N,I>(-1) { }
  VecV ( const string& s, VecVReaderContext& rc ) : StaticSafeArray<N,I>(-1) { read(s,rc); }
  // Specification methods...
  void unOffset    ( const RCType& rc ) { for(int i=0; i<NUM_ENTS; i++) if(StaticSafeArray<N,I>::get(i)!=-1) StaticSafeArray<N,I>::set(i) += -rc.offset; }
  void pushOffsets ( RCType& rc )       { for(typename map<string,int>::iterator i=rc.msi.begin(); i!=rc.msi.end(); i++) i->second+=NUM_ENTS; }
  // Extraction methods...
  //int  getHashConst ( const int m ) const { int k=1; for(int i=0;i<NUM_ENTS;i++){k=(k<<sizeof(I))%m; } return k; }
  //int  getHashKey   ( const int m ) const { int k=0; for(int i=0;i<NUM_ENTS;i++){k=k<<sizeof(I); k+=StaticSafeArray<N,I>::get(i); k%=m; } return k; }
  size_t getHashKey   ( ) const { size_t k=0; for(int i=0;i<NUM_ENTS;i++){k=rotLeft(k,3); k^=StaticSafeArray<N,I>::get(i).toInt(); } return k; }
  bool operator< (const VecV<N,I,RC,ND1,ND2>& v) const { return StaticSafeArray<N,I>::operator<(v);  }
  bool operator==(const VecV<N,I,RC,ND1,ND2>& v) const { return StaticSafeArray<N,I>::operator==(v); }
  void readercontextPush ( VecVReaderContext& rc ) const { rc.offset+=N; }
  void readercontextPop  ( VecVReaderContext& rc ) const { rc.offset-=N; }
  // Input / output methods...
  void read  ( char*, VecVReaderContext& ) ;
  void write ( FILE*, RC& ) const ;
  void write ( FILE* pf ) const { RC rc; write(pf,rc); }
  string getString( RC& ) const;
  string getString() const { RC rc; return getString(rc); }
};
template<int N,class I,class RC,int ND1,int ND2> const int VecV<N,I,RC,ND1,ND2>::NUM_ENTS      = N;
template<int N,class I,class RC,int ND1,int ND2> const int VecV<N,I,RC,ND1,ND2>::NUM_ENTS_DEP1 = ND1;
template<int N,class I,class RC,int ND1,int ND2> const int VecV<N,I,RC,ND1,ND2>::NUM_ENTS_DEP2 = ND2;

////////////////////
template<int N,class I,class RC,int ND1,int ND2>
void VecV<N,I,RC,ND1,ND2>::read ( char* ps, VecVReaderContext& rc ) {
  ////fprintf(stderr,"VecV::VecV in %d\n",rc.offset);
  StaticSafeArray<N,string> asV;

  // Chop into individual coinds strings...
  char* psT; int i=0;
  for ( char* psU=strtok_r(ps,",",&psT);
        psU && i<NUM_ENTS;
        psU=strtok_r(NULL,",",&psT), i++ )
    asV.set(i) = psU;

//  int i;
//  string::size_type j;
////  for ( i=0, j=s.find_first_of(',');  j!=string::npos;  i++, j=s.find_first_of(',',j+1) )
////  for ( i=0, j=-1;  i==0 || j!=string::npos;  i++, j=s.find_first_of(',',j+1) )
//  for ( i=0, j=0;  s!="" && j!=string::npos;  i++, j=s.find_first_of(',',j), j=(-1==j)?j:j+1 )
//    asV.set(i) = s.substr ( j, s.find_first_of(',',j)-j );

  // Going backwards...
  for ( i=i-1; i>=0; i-- ) {
    // Add to map if new coind...
    if (rc.msi.find(asV.get(i)) == rc.msi.end()) rc.msi[asV.get(i)]=i+rc.offset;
    StaticSafeArray<N,I>::set(i)=rc.msi[asV.get(i)];
  }
  ////fprintf(stderr,"VecV::read out\n");
  //for(int i=0; i<NUM_ENTS; i++) fprintf(stderr," %d",int(StaticSafeArray<N,I>::get(i))); fprintf(stderr,"\n");
  //write(stderr);
}

////////////////////
template<int N,class I,class RC,int ND1,int ND2>
void VecV<N,I,RC,ND1,ND2>::write ( FILE* pf, RC& rc ) const {
  for(int i=0; i<NUM_ENTS; i++)
    ////fprintf(pf,"%sv%02d",(i)?",":"",int(StaticSafeArray<N,I>::get(i).toInt()));
    if(StaticSafeArray<N,I>::get(i) >= 0 )
      fprintf(pf,"%sv%02d",(i)?",":"",(rc.offset)+int(StaticSafeArray<N,I>::get(i).toInt()));
}

template<int N,class I,class RC,int ND1,int ND2>
string VecV<N,I,RC,ND1,ND2>::getString (RC& rc) const {
  string rString;
  for(int i=0; i<NUM_ENTS; i++)
    if(StaticSafeArray<N,I>::get(i) >= 0 ) {
      rString += (i)?",":"";
      char* tmp = new char[4];
      //      char tmp* = (char*)malloc(4*sizeof(char));
      sprintf(tmp, "v%02d\0",(rc.offset)+int(StaticSafeArray<N,I>::get(i).toInt()));
      rString += tmp;
      delete tmp;
      // free(tmp);
    }
  return rString;
}

////////////////////////////////////////////////////////////////////////////////
//
//  VecV Compositors
//
////////////////////////////////////////////////////////////////////////////////

template<class V1, class V2>
class JointVecV { //// : public StaticSafeArray<V1::NUM_ENTS+V2::NUM_ENTS,I> {
 private:
  V1 v1;
  V2 v2;
 public:
  typedef typename V1::IntType IntType;
  typedef typename V1::RCType  RCType;
  static const int NUM_ENTS;
  // Constructor / destructor methods...
  JointVecV ( ) { }
  JointVecV ( const V1& a1, const V2& a2 ) {
    ////fprintf(stderr,"iJoin "); a1.V1::write(stderr); fprintf(stderr," "); a2.V2::write(stderr); fprintf(stderr,"\n");
    for (int i=0; i<NUM_ENTS; i++) {
      if ( i<V1::NUM_ENTS ) set(i) = (a1.get(i)==-1)              ? IntType(-1) : (a1.get(i)<V1::NUM_ENTS) ? IntType(a1.get(i)) : a1.get(i)+V2::NUM_ENTS;
      else                  set(i) = (a2.get(i-V1::NUM_ENTS)==-1) ? IntType(-1) : a2.get(i-V1::NUM_ENTS)+V1::NUM_ENTS;
    }
    ////fprintf(stderr,"oJoin "); JointVecV<V1,V2>::write(stderr); fprintf(stderr,"\n");
  }
  // Specification methods...
  typename V1::IntType& set(int i) { assert(i>=0); assert(i<NUM_ENTS); return ((i<V1::NUM_ENTS) ? v1.set(i) : v2.set(i-V1::NUM_ENTS) ); }
  V1&                   setSub1()  { return v1; }
  V2&                   setSub2()  { return v2; }
  void                  unOffset    ( const RCType& rc ) { setSub1().unOffset(rc); setSub2().unOffset(rc); }
  void                  pushOffsets ( RCType& rc )       { for(typename map<string,int>::iterator i=rc.msi.begin(); i!=rc.msi.end(); i++) i->second+=NUM_ENTS; }
  // Extraction methods...
  //int       getHashConst ( const int m ) const { return((v1.getHashConst(m) * v2.getHashConst(m))%m); }
  //int       getHashKey   ( const int m ) const { int k=((v1.getHashKey(m)*v2.getHashConst(m))+v2.getHashKey(m))%m; return k; }
  size_t getHashKey   ( ) const { size_t k=rotLeft(v1.getHashKey(),3); k^=v2.getHashKey(); return k; }
  bool                       operator==(const JointVecV<V1,V2>& vv) const { return (v1==vv.v1 && v2==vv.v2); }
  bool                       operator<(const JointVecV<V1,V2>& vv) const { return (v1<vv.v1 || (v1==vv.v1 && v2<vv.v2)); }
  void readercontextPush ( VecVReaderContext& rc ) const { rc.offset+=NUM_ENTS; }
  void readercontextPop  ( VecVReaderContext& rc ) const { rc.offset-=NUM_ENTS; }
  const typename V1::IntType get(int i) const { assert(i>=0); assert(i<NUM_ENTS); return ((i<V1::NUM_ENTS) ? v1.get(i) : v2.get(i-V1::NUM_ENTS) ); }
  const V1&                  getSub1() const { return v1; }
  const V2&                  getSub2() const { return v2; }
  // Input / output methods...
  void write ( FILE* pf, RCType& rc )  const { v1.write(pf,rc); /*JointVecV<V1,V2>::getSub1().readercontextPush(rc);*/ fprintf(pf,";");
                                               v2.write(pf,rc); /*JointVecV<V1,V2>::getSub1().readercontextPop(rc);*/ }
  void write ( FILE* pf ) const { RCType rc; write(pf,rc); }
  string getString( RCType& rc ) const { return v1.getString(rc) + ";" + v2.getString(rc); }
  string getString() const { RCType rc; return getString(rc); }
};
template<class V1, class V2>
const int JointVecV<V1,V2>::NUM_ENTS = V1::NUM_ENTS+V2::NUM_ENTS;

////////////////////////////////////////////////////////////

template<char* SD1,class V1,char* SD2,class V2,char* SD3>
class DelimitedJointVecV : public JointVecV<V1,V2> {
 public:
  typedef typename V1::RCType RCType;
  // Constructor / destructor methods...
  DelimitedJointVecV ( )                                          : JointVecV<V1,V2>() { }
  DelimitedJointVecV ( const string& s, typename V1::RCType& rc ) : JointVecV<V1,V2>() { read(s,rc); }
  // Extraction methods...
  bool operator==(const DelimitedJointVecV<SD1,V1,SD2,V2,SD3>& vv) const { return JointVecV<V1,V2>::operator==(vv); }
  bool operator<(const DelimitedJointVecV<SD1,V1,SD2,V2,SD3>& vv) const { return JointVecV<V1,V2>::operator<(vv); }
  void readercontextPush ( typename V1::RCType& rc ) const { JointVecV<V1,V2>::getSub1().readercontextPush(rc); JointVecV<V1,V2>::getSub2().readercontextPush(rc); }
  void readercontextPop  ( typename V1::RCType& rc ) const { JointVecV<V1,V2>::getSub1().readercontextPop(rc);  JointVecV<V1,V2>::getSub2().readercontextPop(rc);  }
  // Input / output methods...
  void read  ( char*, RCType& ) ;
  void write ( FILE* pf, RCType& rc ) const {
    fprintf(pf,"%s",SD1); JointVecV<V1,V2>::getSub1().write(pf,rc); /*JointVecV<V1,V2>::getSub1().readercontextPush(rc);*/
    fprintf(pf,"%s",SD2); JointVecV<V1,V2>::getSub2().write(pf,rc); /*JointVecV<V1,V2>::getSub1().readercontextPop(rc);*/
    fprintf(pf,"%s",SD3);
  }
  void write ( FILE* pf ) const { RCType rc; write(pf,rc); }
  string getString(RCType& rc) const {
     return SD1 + JointVecV<V1,V2>::getSub1().getString(rc) + SD2 + JointVecV<V1,V2>::getSub2().getString(rc) + SD3;
  }
  string getString() const { RCType rc; return getString(rc); }
};

////////////////////
template<char* SD1,class V1,char* SD2,class V2,char* SD3>
void DelimitedJointVecV<SD1,V1,SD2,V2,SD3>::read ( char* s, RCType& rc ) {
  ////fprintf(stderr,"DelimitedJointVecV::read chopping '%s' into '%s'...'%s'...'%s'\n",s.c_str(),SD1,SD2,SD3);
  if(0!=strncmp(SD1,s,strlen(SD1)))           fprintf(stderr,"ERR: '%s' doesn't begin with '%s'\n",s,SD1);
  if(0!=strcmp(SD3,s+strlen(s)-strlen(SD3)))  fprintf(stderr,"ERR: '%s' doesn't end with '%s'\n",  s,SD3);
  /*
  if(0!=strncmp(SD1,s.c_str(),strlen(SD1))) fprintf(stderr,"ERR: '%s' doesn't begin with '%s'\n",s.c_str(),SD1);
  if(0!=strcmp(SD3,s.c_str()+s.length()-strlen(SD3)))  fprintf(stderr,"ERR: '%s' doesn't end with '%s'\n",s.c_str(),SD3);
  string::size_type        j  = s.find(SD2);
  if(string::npos==j)                       fprintf(stderr,"ERR: no '%s' found in %s\n",SD2,s.c_str());
  string                   s1 = s.substr ( strlen(SD1), j-strlen(SD1) );
  string                   s2 = s.substr ( j+strlen(SD2), int(s.length())-int(j)-strlen(SD2)-strlen(SD3) );
  */
  char* s1 = s+strlen(SD1);
  char* s2 = strstr(s,SD2); if (!s2) fprintf(stderr,"WARNING: (VecV) no '%s' found in '%s' -- assuming second string empty\n",SD2,s);
  s[strlen(s)-strlen(SD3)]='\0';
  if (s2) { *s2='\0'; s2+=strlen(SD2); } else { s2=s+strlen(s); }
  //fprintf(stderr,"s1=%s, s2=%s\n",s1,s2);
  ////fprintf(stderr,"DelimitedJointVecV::read    gives `%s' and `%s'\n",s1.c_str(),s2.c_str());
  JointVecV<V1,V2>::getSub1().readercontextPush(rc); JointVecV<V1,V2>::setSub2().read ( s2, rc ); //V1::NUM_ENTS, msi );
  JointVecV<V1,V2>::getSub1().readercontextPop(rc);  JointVecV<V1,V2>::setSub1().read ( s1, rc ); //0,            msi );
}

////////////////////////////////////////////////////////////

char psXX[] = "";       // Null delimiter for joint RVs
template<char N>
class VecVV : public DelimitedJointVecV<psXX,VecV<N,Id<char> >,psXX,VecV<N,Id<char> >,psXX> {};       // Coindexation vector, for result of binary branch rule

////////////////////////////////////////////////////////////////////////////////

template<class V1, class V2>
class ComposedVecV : public JointVecV<V1,V2> {
 public:
  ComposedVecV ( const V1& v1, const V2& v2 ) : JointVecV<V1,V2>()  {
    ////fprintf(stderr,"iComp "); v1.write(stderr); fprintf(stderr," "); v2.write(stderr); fprintf(stderr,"\n");
    for (int i=0; i<V1::NUM_ENTS+V2::NUM_ENTS; i++) {
      // If in first sub-part...
      if ( i<V1::NUM_ENTS ) {
        // If not valid...
        if ( v1.get(i)==-1 ) continue;
        // If v1[i] intra-indexed...
        else if ( v1.get(i)<V1::NUM_ENTS )
          JointVecV<V1,V2>::set(i) = v1.get(i);
        // If v1[i] inter-indexed...
        else if ( v1.get(i)<V1::NUM_ENTS+V2::NUM_ENTS )
//{
          JointVecV<V1,V2>::set(i) = (v2.get(v1.get(i).toInt()-V1::NUM_ENTS)!=-1) ? v2.get(v1.get(i).toInt()-V1::NUM_ENTS)+V1::NUM_ENTS : -1;
//fprintf(stderr,"??????????? %d(%d,%d)%d,%d: ",i,V1::NUM_ENTS,V2::NUM_ENTS,int(v1.get(i)),int(v2.get(13))); JointVecV<V1,V2>::write(stderr); fprintf(stderr,"\n");
//}
        else
          JointVecV<V1,V2>::set(i) = v1.get(i);
      }
      else {
        // If not valid...
        if ( v2.get(i-V1::NUM_ENTS)==-1 ) continue;
        // If in second sub-part...
        else JointVecV<V1,V2>::set(i) = v2.get(i-V1::NUM_ENTS)+V1::NUM_ENTS;
      }
    }
    ////fprintf(stderr,"oComp "); JointVecV<V1,V2>::write(stderr); fprintf(stderr,"\n");
  }
};

////////////////////////////////////////////////////////////////////////////////

template<class V1, class V2>
class CommutedVecV : public JointVecV<V1,V2> {
 public:
  CommutedVecV ( const JointVecV<V2,V1>& v ) : JointVecV<V1,V2>()  {
    ////fprintf(stderr,"iComm "); v.write(stderr); fprintf(stderr,"\n");
    // Iterate backward through new coind vector...
    for ( int i=V1::NUM_ENTS+V2::NUM_ENTS-1; i>=0; i-- ) {
      // If in second sub part V2 (first in v)...
      if ( i>=V1::NUM_ENTS ) {
        // If not valid...
        if ( v.get(i-V1::NUM_ENTS)==-1 ) continue;
        // If v[i-w2] intra-indexed...
        else if ( v.get(i-V1::NUM_ENTS)<V2::NUM_ENTS )
          JointVecV<V1,V2>::set(i) = v.get(i-V1::NUM_ENTS)+V1::NUM_ENTS;
        // If v[i-w2] cross-indexed to other sub-part (V1)...
        else if ( v.get(i-V1::NUM_ENTS)<V2::NUM_ENTS+V1::NUM_ENTS ) {
          if (get(v.get(i-V1::NUM_ENTS).toInt()-V2::NUM_ENTS)==-1)
            JointVecV<V1,V2>::set(v.get(i-V1::NUM_ENTS).toInt()-V2::NUM_ENTS) = i;
          JointVecV<V1,V2>::set(i) = get(v.get(i-V1::NUM_ENTS).toInt()-V2::NUM_ENTS);
        }
        // If v[i-w2] inter-indexed...
        else JointVecV<V1,V2>::set(i) = v.get(i-V1::NUM_ENTS);
      }
      // If in first sub part V1 (second in v), and not coindexed already...
      else if (JointVecV<V1,V2>::get(i)==-1) {
        // If not valid...
        if ( v.get(i+V2::NUM_ENTS)==-1 ) continue;
        // If v[i+w1] intra-indexed...
        else if ( v.get(i+V2::NUM_ENTS)<V2::NUM_ENTS+V1::NUM_ENTS ) {
          if ( get(v.get(i+V2::NUM_ENTS).toInt()-V2::NUM_ENTS)==-1 )                       // should only happen if self-coindexed
            JointVecV<V1,V2>::set(i) = v.get(i+V2::NUM_ENTS)-V2::NUM_ENTS;
          else
            JointVecV<V1,V2>::set(i) = get(v.get(i+V2::NUM_ENTS).toInt()-V2::NUM_ENTS);
        }
        // If v[i+w1] inter-indexed...
        else JointVecV<V1,V2>::set(i) = v.get(i+V2::NUM_ENTS);
      }
      //fprintf(stderr,"??????????? %d: ",i); v.write(stderr); fprintf(stderr," -> "); JointVecV<V1,V2>::write(stderr); fprintf(stderr,"\n");
    }
    ////fprintf(stderr,"oComm "); JointVecV<V1,V2>::write(stderr); fprintf(stderr,"\n");
  }
};

////////////////////////////////////////////////////////////////////////////////

template<class V1, class V2, class V3>
class NewCommutedVecV : public JointVecV<V1,JointVecV<V2,V3> > {
 public:
  NewCommutedVecV ( const JointVecV<V1,JointVecV<V3,V2> >& v ) : JointVecV<V1,JointVecV<V2,V3> >() {
    ////fprintf(stderr,"iNewComm "); v.write(stderr); fprintf(stderr,"\n");
    // Iterate backward through new coind vector...
    for ( int i=V1::NUM_ENTS+V2::NUM_ENTS+V3::NUM_ENTS-1; i>=0; i-- ) {
      // If in third sub part V3 (second in v)...
      if ( i>=V1::NUM_ENTS+V2::NUM_ENTS ) {
        // If not valid...
        if ( v.get(i-V2::NUM_ENTS)==-1 ) continue;
        // If v[i-w2] intra-indexed...
        else if ( v.get(i-V2::NUM_ENTS)<V1::NUM_ENTS+V3::NUM_ENTS )
          JointVecV<V1,JointVecV<V2,V3> >::set(i) = v.get(i-V2::NUM_ENTS)+V2::NUM_ENTS;
        // If v[i-w2] cross-indexed to other sub-part (V2, third in v)...
        else if ( v.get(i-V2::NUM_ENTS)<V1::NUM_ENTS+V3::NUM_ENTS+V2::NUM_ENTS ) {
          if (get(v.get(i-V2::NUM_ENTS).toInt()-V3::NUM_ENTS)==-1)
            JointVecV<V1,JointVecV<V2,V3> >::set(v.get(i-V2::NUM_ENTS).toInt()-V3::NUM_ENTS) = i;
          JointVecV<V1,JointVecV<V2,V3> >::set(i) = get(v.get(i-V2::NUM_ENTS).toInt()-V3::NUM_ENTS);
        }
        // If v[i-w2] inter-indexed...
        else JointVecV<V1,JointVecV<V2,V3> >::set(i) = v.get(i-V2::NUM_ENTS);
      }
      // If in second sub part V2 (third in v), and not coindexed already...
      else if ( i>=V1::NUM_ENTS && JointVecV<V1,JointVecV<V2,V3> >::get(i)==-1 ) {
        // If not valid...
        if ( v.get(i+V3::NUM_ENTS)==-1 ) continue;
        // If v[i+w1] intra-indexed...
        else if ( v.get(i+V3::NUM_ENTS)<V1::NUM_ENTS+V3::NUM_ENTS+V2::NUM_ENTS )
          if ( get(v.get(i+V3::NUM_ENTS).toInt()-V3::NUM_ENTS)==-1 )                       // should only happen if self-coindexed
            JointVecV<V1,JointVecV<V2,V3> >::set(i) = v.get(i+V3::NUM_ENTS)-V3::NUM_ENTS;
          else
            JointVecV<V1,JointVecV<V2,V3> >::set(i) = get(v.get(i+V3::NUM_ENTS).toInt()-V3::NUM_ENTS);
        // If v[i+w1] inter-indexed...
        else JointVecV<V1,JointVecV<V2,V3> >::set(i) = v.get(i+V3::NUM_ENTS);
      }
      // If in first sub part V1 (also first in v)...
      else if ( i<V1::NUM_ENTS ) {
        // If not valid...
        if ( v.get(i)==-1 ) continue;
        // If v[i] intra-indexed to V1 or inter-indexed to cond...
        else if ( v.get(i)<V1::NUM_ENTS || v.get(i)>=V1::NUM_ENTS+V3::NUM_ENTS+V2::NUM_ENTS )
          JointVecV<V1,JointVecV<V2,V3> >::set(i) = v.get(i);
        // If v[i] inter-indexed to V3 (second in v)...
        else if ( v.get(i)<V1::NUM_ENTS+V3::NUM_ENTS )
          JointVecV<V1,JointVecV<V2,V3> >::set(i) = v.get(i)+V2::NUM_ENTS;
        // If v[i] inter-indexed to V2 (third in v) (note: may have gotten forwarded in commute)...
        else JointVecV<V1,JointVecV<V2,V3> >::set(i) = get(v.get(i).toInt()-V3::NUM_ENTS);
      }
      //fprintf(stderr,"??????????? %d: ",i); v.write(stderr); fprintf(stderr," -> "); JointVecV<V1,JointVecV<V2,V3> >::write(stderr); fprintf(stderr,"\n");
    }
    ////fprintf(stderr,"oNewComm\n"); //JointVecV<V2,V3>::write(stderr); fprintf(stderr,"\n");
  }
};

////////////////////////////////////////////////////////////////////////////////

template<class V, class V1>
class AssociatedVecV : public V {
 public:
  AssociatedVecV ( const V1& v ) {
    // Iterate through new coind vector...
    for ( int i=0; i<V::NUM_ENTS; i++ )
      V::set(i) = v.get(i);
  }
};

////////////////////////////////////////

template<class V1, class V2>
class MarginalVecV : public V1 {
 public:
  MarginalVecV ( const JointVecV<V1,V2>& v ) {
    ////fprintf(stderr,"iMarg "); v.write(stderr); fprintf(stderr,"\n");
    // Iterate through new coind vector...
    for ( int i=0; i<V1::NUM_ENTS; i++ ) {
      // If not valid...
      if ( v.get(i)==-1 ) continue;
      // If intra-coref...
      else if (v.get(i)<V1::NUM_ENTS) V1::set(i)=v.get(i);
      // If inter-coref to marginalized var (assume target value must be self-coref or first coref would point there)...
      else if (v.get(i)<V1::NUM_ENTS+V2::NUM_ENTS) {
        for (int j=0; j<=i; j++)
          if (v.get(j)==v.get(i)) V1::set(j)=i;
      }
      // If inter-coref to var after marginalized var...
      else V1::set(i)=v.get(i)-V2::NUM_ENTS;
    }
    ////fprintf(stderr,"oMarg "); V1::write(stderr); fprintf(stderr,"\n");
  }
};

#endif /* _NL_DENOT__ */
