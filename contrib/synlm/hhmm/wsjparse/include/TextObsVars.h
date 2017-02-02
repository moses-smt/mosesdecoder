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

#ifndef _TEXT_OBS_VARS_
#define _TEXT_OBS_VARS_

#include "nl-randvar.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Random Variables
//
////////////////////////////////////////////////////////////////////////////////

//// P: part of speech category...
DiscreteDomain<short> domainP;
typedef DiscreteDomainRV<short,domainP> P;

//// L: letter...
DiscreteDomain<char> domainLt;
typedef DiscreteDomainRV<char,domainLt> Lt;

//// X: observed word (array of letters, arranged last to first)...
//typedef StaticSafeArray<5,Lt> X;
DiscreteDomain<int> domainX;
class X : public DiscreteDomainRV<int,domainX>, public StaticSafeArray<5,Lt> {
 public:
  X ( ) { }
  X ( const char* ps ) : DiscreteDomainRV<int,domainX>(ps) {
    char psTemp[2]="-"; int n=strlen(ps);
    for(int i=0;i<5;i++) {
      psTemp[0]=(i<n)?ps[n-i-1]:'_';
      //cout<<"!!!!!!!!!!!!!!!!!!"<<psTemp<<endl;
      StaticSafeArray<5,Lt>::set(i)=Lt(psTemp);
    }
  }
  friend pair<StringInput,X*> operator>> ( const StringInput ps, X& rv ) { return pair<StringInput,X*>(ps,&rv); }
  friend StringInput operator>> ( pair<StringInput,X*> si_x, const char* psDlm ) {
    if(si_x.first==NULL)return si_x.first; String s; StringInput si=si_x.first>>s>>psDlm; *si_x.second=s.c_array(); return si; }
  bool   operator== ( const X& x ) const { return DiscreteDomainRV<int,domainX>::operator==(x); }
  size_t getHashKey ( )            const { return DiscreteDomainRV<int,domainX>::getHashKey();  }
};

//// W: subset of words with reliable statistics for POS model
DiscreteDomain<int> domW;
class W : public DiscreteDomainRV<int,domW> {
 private:
  static SimpleHash<X,W> hXtoW;
  void calcDetModels ( string s ) { if (!hXtoW.contains(X(s.c_str()))) hXtoW.set(X(s.c_str())) = *this; }
 public:
  static const W W_UNK;
  W ( )                                      : DiscreteDomainRV<int,domW> ( )    { }
  W ( const DiscreteDomainRV<int,domW>& rv ) : DiscreteDomainRV<int,domW>(rv) { }
  W ( const char* ps )                       : DiscreteDomainRV<int,domW> ( ps ) { calcDetModels(ps); }
  //C ( string s ) : DiscreteDomainRV<int,domC> ( s )  { calcDetModels(s); }
  W ( const X& x ) { *this = (hXtoW.contains(x)) ? hXtoW.get(x) : W_UNK; }
  friend pair<StringInput,W*> operator>> ( StringInput si, W& x ) { return pair<StringInput,W*>(si,&x); }
  friend StringInput operator>> ( pair<StringInput,W*> si_x, const char* psD ) {
    if ( si_x.first == NULL ) return NULL;
    StringInput si=si_x.first>>(DiscreteDomainRV<int,domW>&)*si_x.second>>psD;
    si_x.second->calcDetModels(si_x.second->getString()); return si; }
};
SimpleHash<X,W> W::hXtoW;
const W W::W_UNK ("unk");
const W W_UNK = W::W_UNK;

//// H: subset of words within threshhold of head words used for clustering
DiscreteDomain<int> domH;
class H : public DiscreteDomainRV<int,domH> {
 private:
  static SimpleHash<X,H> hXtoH;
  void calcDetModels ( string s ) { if (!hXtoH.contains(X(s.c_str()))) hXtoH.set(X(s.c_str())) = *this; }
 public:
  static const H H_UNK;
  H ( )                                      : DiscreteDomainRV<int,domH> ( )    { }
  H ( const DiscreteDomainRV<int,domH>& rv ) : DiscreteDomainRV<int,domH>(rv) { }
  H ( const char* ps )                       : DiscreteDomainRV<int,domH> ( ps ) { calcDetModels(ps); }
  //C ( string s ) : DiscreteDomainRV<int,domC> ( s )  { calcDetModels(s); }
  H ( const X& x ) { *this = (hXtoH.contains(x)) ? hXtoH.get(x) : H_UNK; }
  friend pair<StringInput,H*> operator>> ( StringInput si, H& x ) { return pair<StringInput,H*>(si,&x); }
  friend StringInput operator>> ( pair<StringInput,H*> si_x, const char* psD ) {
    if ( si_x.first == NULL ) return NULL;
    StringInput si=si_x.first>>(DiscreteDomainRV<int,domH>&)*si_x.second>>psD;
    si_x.second->calcDetModels(si_x.second->getString()); return si; }
};
SimpleHash<X,H> H::hXtoH;
const H H::H_UNK ("unk");
const H H_UNK = H::H_UNK;

#endif //_TEXT_OBS_VARS_
