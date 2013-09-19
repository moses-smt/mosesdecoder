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

#ifndef _NL_STREAM__
#define _NL_STREAM__

#include "nl-string.h"


////////////////////////////////////////////////////////////////////////////////

class IStreamSource {
 private:

  // Data members...
  String       sBuff;  // string buffer, to allow re-reading in disjunctions
  unsigned int iAvail; // index of where string buffer begins; subtract from indices in IStream to get buffer index
  FILE*        pfSrc;  // file pointer

 public:

  IStreamSource ( FILE* pf )        : sBuff(256), iAvail(0), pfSrc(pf) { }
  IStreamSource ( const char* psz ) : sBuff(psz), iAvail(0), pfSrc()   { }

  char* c_array ( )                { return sBuff.c_array(); }
  char  get     ( unsigned int i ) { assert(i-iAvail<((unsigned int)(-1))/2);  // NOTE: PROBLEM IF ANY ISTREAM FALLS BEHIND BY MORE THAN MAXINT CHARACTERS!
                                     int c=0; while(i-iAvail>=sBuff.size()-1){sBuff.add()=((c=getc(pfSrc))==EOF)?'\0':c;} return sBuff.get(i-iAvail); }
  char& set     ( unsigned int i ) { assert(i-iAvail<((unsigned int)(-1))/2);  // NOTE: PROBLEM IF ANY ISTREAM FALLS BEHIND BY MORE THAN MAXINT CHARACTERS!
                                     int c=0; while(i-iAvail>=sBuff.size()-1){sBuff.add()=((c=getc(pfSrc))==EOF)?'\0':c;} return sBuff.set(i-iAvail); }
  FILE* getFile ( )                { return pfSrc; }

  // Output method...
  friend ostream& operator<< ( ostream& os, const IStreamSource& iss ) { return os<<"'"<<iss.sBuff<<"',"<<iss.iAvail<<","<<iss.pfSrc; }

  void compress ( ) { iAvail+=strlen(sBuff.c_array()); sBuff=String(256); }    // NOTE: NOT TESTED WITH FILES EXCEEDING MAXINT CHARACTERS! (BUT SHOULD WORK)
};


////////////////////////////////////////////////////////////////////////////////

class IStream {
 private:

  IStreamSource* psrc;   // pointer to source of stream, which contains buffer and file pointer
  unsigned int   iIndex; // stream index; subtract psrc->iAvail to get index in psrc->sBuff

  char  get     ( unsigned int i )   { return psrc->get(i);  }
  char& set     ( unsigned int i )   { return psrc->set(i);  }
  char* c_array ( unsigned int i=0 ) { return &psrc->set(i); }

 public:

  IStream ( )                                      : psrc(NULL),    iIndex(0) { }
  IStream ( IStreamSource& src, unsigned int i=0 ) : psrc(&src),    iIndex(i) { }
  IStream ( const IStream& is,  unsigned int i   ) : psrc(is.psrc), iIndex(i) { }

  // Specification methods...
  IStream& operator= ( const IStream& is ) { psrc=is.psrc; iIndex=is.iIndex; return *this; }

  // Extraction methods...
  bool operator== ( const IStream& is ) const { return (psrc==is.psrc && iIndex==is.iIndex); }
  bool operator!= ( const IStream& is ) const { return (psrc!=is.psrc || iIndex!=is.iIndex); }
  IStreamSource* getSource() { return psrc; }

  // Output method...
  friend ostream& operator<< ( ostream& os, const IStream& is ) { return os<<is.iIndex<<","<<is.psrc<<","<<*is.psrc; }

  // Match single char...
  friend IStream operator>> ( IStream is, char& c ) { 
    // Propagate fail...
    if (IStream()==is) return IStream();
    c=is.get(is.iIndex);
    return ('\0'==c) ? IStream() : IStream(is,is.iIndex+1);
  }

  // Match string delimiter...
  friend IStream operator>> ( IStream is, const char* psDlm ) {
    // Propagate fail...
    if (IStream()==is) { return IStream(); }
    unsigned int i;
    // Read in characters until fail or match delimiter...
    for (i=0; is.get(i+is.iIndex)!='\0' && psDlm[i]!='\0'; i++)
      if (is.get(i+is.iIndex)!=psDlm[i]) return IStream();
    return (psDlm[i]=='\0') ? IStream(is,i+is.iIndex) : IStream();
  }

  // Match anything else followed by zero-terminated string delimiter...
  template<class X> friend pair<IStream,X*> operator>> ( IStream is, X& x ) { return pair<IStream,X*>(is,&x); }
  template<class X> friend IStream          operator>> ( pair<IStream,X*> is_x, const char* psDlm ) { 
    IStream& is =  is_x.first;
    X&       x  = *is_x.second;
    // Propagate fail...
    if (IStream()==is) { return IStream(); }
    unsigned int d = strlen(psDlm);
    // Read in characters until fail or match delimiter...
    for (unsigned int i=is.iIndex; is.get(i)!='\0'; i++) {
      // Try to match delimiter ending at current point in stream...
      if ( i-is.iIndex+1>=d ) {
        unsigned int j;
        for (j=0; j<d && is.get(i+1+j-d)==psDlm[j]; j++);
        ////cerr<<"-->i="<<i<<",j="<<j<<",d="<<d<<",delim='"<<psDlm<<"',curr='"<<is.get(i)<<"'  "<<is<<"\n";
        if (j==d) {
          is.set(i+1-d)='\0'; x=X(is.c_array(is.iIndex)); is.set(i+1-d)=psDlm[0];
          return IStream(is,i+1);
        }
      }
    }
    return IStream();
  }

  // Match integer followed by zero-terminated string delimiter...
  friend IStream operator>> ( pair<IStream,int*> is_x, const char* psDlm ) { 
    IStream& is =  is_x.first;
    int&     x  = *is_x.second;
    // Propagate fail...
    if (IStream()==is) return IStream();
    unsigned int d = strlen(psDlm);
    // Read in characters until fail or match delimiter...
    for (unsigned int i=is.iIndex; is.get(i)!='\0'; i++) {
      // Try to match delimiter ending at current point in stream...
      if ( i+1>=is.iIndex+d ) {
        unsigned int j;
        for (j=0; j<d && is.get(i+1+j-d)==psDlm[j]; j++);
        if (j==d) {
          is.set(i+1-d)='\0'; x=atoi(is.c_array(is.iIndex)); is.set(i+1-d)=psDlm[0];
          return IStream(is,i+1);
        }
      }
    }
    return IStream();
  }

  // Match unsigned int followed by zero-terminated string delimiter...
  friend IStream operator>> ( pair<IStream,unsigned int*> is_x, const char* psDlm ) { 
    IStream& is     =  is_x.first;
    unsigned int& x = *is_x.second;
    // Propagate fail...
    if (IStream()==is) return IStream();
    unsigned int d = strlen(psDlm);
    // Read in characters until fail or match delimiter...
    for (unsigned int i=is.iIndex; is.get(i)!='\0'; i++) {
      // Try to match delimiter ending at current point in stream...
      if ( i+1>=is.iIndex+d ) {
        unsigned int j;
        for (j=0; j<d && is.get(i+1+j-d)==psDlm[j]; j++);
        if (j==d) {
          is.set(i+1-d)='\0'; x=atoi(is.c_array(is.iIndex)); is.set(i+1-d)=psDlm[0];
          return IStream(is,i+1);
        }
      }
    }
    return IStream();
  }

  // Match float followed by zero-terminated string delimiter...
  friend IStream operator>> ( pair<IStream,float*> is_x, const char* psDlm ) { 
    IStream& is =  is_x.first;
    float&  x  = *is_x.second;
    // Propagate fail...
    if (IStream()==is) return IStream();
    unsigned int d = strlen(psDlm);
    // Read in characters until fail or match delimiter...
    for (unsigned int i=is.iIndex; is.get(i)!='\0'; i++) {
      // Try to match delimiter ending at current point in stream...
      if ( i+1>=is.iIndex+d ) {
        unsigned int j;
        for (j=0; j<d && is.get(i+1+j-d)==psDlm[j]; j++);
        if (j==d) {
          is.set(i+1-d)='\0'; x=atof(is.c_array(is.iIndex)); is.set(i+1-d)=psDlm[0];
          return IStream(is,i+1);
        }
      }
    }
    return IStream();
  }

  // Match double followed by zero-terminated string delimiter...
  friend IStream operator>> ( pair<IStream,double*> is_x, const char* psDlm ) { 
    IStream& is =  is_x.first;
    double&  x  = *is_x.second;
    // Propagate fail...
    if (IStream()==is) return IStream();
    unsigned int d = strlen(psDlm);
    // Read in characters until fail or match delimiter...
    for (unsigned int i=is.iIndex; is.get(i)!='\0'; i++) {
      // Try to match delimiter ending at current point in stream...
      if ( i+1>=is.iIndex+d ) {
        unsigned int j;
        for (j=0; j<d && is.get(i+1+j-d)==psDlm[j]; j++);
        if (j==d) {
          is.set(i+1-d)='\0'; x=atof(is.c_array(is.iIndex)); is.set(i+1-d)=psDlm[0];
          return IStream(is,i+1);
        }
      }
    }
    return IStream();
  }

  // Match void pointer followed by zero-terminated string delimiter...
  friend IStream operator>> ( pair<IStream,void**> is_x, const char* psDlm ) { 
    IStream& is =  is_x.first;
    // Propagate fail...
    if (IStream()==is) return IStream();
    unsigned int d = strlen(psDlm);
    // Read in characters until fail or match delimiter...
    for (unsigned int i=is.iIndex; is.get(i)!='\0'; i++) {
      // Try to match delimiter ending at current point in stream...
      if ( i+1>=is.iIndex+d ) {
        unsigned int j;
        for (j=0; j<d && is.get(i+1+j-d)==psDlm[j]; j++);
        if (j==d) return IStream(is,i+1);
      }
    }
    return IStream();
  }
};


#endif //_NL_STREAM__

