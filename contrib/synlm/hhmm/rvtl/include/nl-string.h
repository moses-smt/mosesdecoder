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

#ifndef _NL_STRING__
#define _NL_STRING__

#include "nl-array.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
using namespace std;

////////////////////////////////////////////////////////////////////////////////


// OBSOLETE!
class StringInput {
 private:
  char* psIn;
  static char* PS_EOS;
 public:

  static const StringInput SI_EOS;
  static Array<char> strTemp;

  StringInput ( )           : psIn(NULL)        { }
  StringInput ( char* ps  ) : psIn(ps)          { }
  ////StringInput ( String& s ) : psIn(s.c_array()) { }

  char&             operator[] ( int i )       { assert(psIn); return psIn[i]; }
  const char        operator[] ( int i ) const { assert(psIn); return psIn[i]; }
  StringInput&      operator++ ( )             { assert(psIn); psIn++; return *this; }
  StringInput       operator+  ( int i )       { assert(psIn); return StringInput(psIn+i); }
  const StringInput operator+  ( int i ) const { assert(psIn); return StringInput(psIn+i); }
  bool              operator== ( const StringInput& si ) const { return psIn==si.psIn; }
  bool              operator!= ( const StringInput& si ) const { return psIn!=si.psIn; }

  //operator bool() { return psIn!=NULL; }

  char* c_str() { assert(psIn); return psIn; }

  friend ostream& operator<< ( ostream& os, const StringInput& si ) { return os<<si.psIn; }

  friend StringInput            operator>> ( StringInput psIn, const char* psDlm ) {
    if (StringInput(NULL)==psIn) return psIn;
    int i;
    for (i=0; psIn[i]!='\0' && psDlm[i]!='\0'; i++) 
      if(psIn[i]!=psDlm[i]) return StringInput(NULL); //psIn;
    return (psDlm[i]!='\0') ? StringInput(NULL) : (psIn[i]!='\0') ? psIn+i : SI_EOS;
  }

  friend pair<StringInput,int*> operator>> ( StringInput ps,   int& n         ) { return pair<StringInput,int*>(ps,&n); }
  friend StringInput            operator>> ( pair<StringInput,int*> delimbuff, const char* psDlm ) { 
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    ///int i; for(i=0;psIn[i]!='\0';i++) if(psIn[i]==psDlm[i]) return psIn; return psIn+i;
    int j=0;
    StringInput psIn = delimbuff.first;
    if(psDlm[0]=='\0') { *delimbuff.second=atoi(psIn.c_str()); return psIn+strlen(psIn.c_str()); }
    for(int i=0;psIn[i]!='\0';i++) {
      if(psIn[i]==psDlm[j]) j++;
      else j=0;
      strTemp[i]=psIn[i];
      if(j==int(strlen(psDlm))) { strTemp[i+1-j]='\0'; *delimbuff.second=atoi(strTemp.c_array()); return psIn+i+1;}
    }
    return NULL; //psIn;
  }

  friend pair<StringInput,unsigned int*> operator>> ( StringInput ps,   unsigned int& n ) { return pair<StringInput,unsigned int*>(ps,&n); }
  friend StringInput            operator>> ( pair<StringInput,unsigned int*> delimbuff, const char* psDlm ) { 
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    ///int i; for(i=0;psIn[i]!='\0';i++) if(psIn[i]==psDlm[i]) return psIn; return psIn+i;
    int j=0;
    StringInput psIn = delimbuff.first;
    if(psDlm[0]=='\0') { *delimbuff.second=atoi(psIn.c_str()); return psIn+strlen(psIn.c_str()); }
    for(int i=0;psIn[i]!='\0';i++) {
      if(psIn[i]==psDlm[j]) j++;
      else j=0;
      strTemp[i]=psIn[i];
      if(j==int(strlen(psDlm))) { strTemp[i+1-j]='\0'; *delimbuff.second=atoi(strTemp.c_array()); return psIn+i+1;}
    }
    return NULL; //psIn;
  }

  friend pair<StringInput,double*> operator>> ( StringInput ps,   double& d         ) { return pair<StringInput,double*>(ps,&d); }
  friend StringInput               operator>> ( pair<StringInput,double*> delimbuff, const char* psDlm ) { 
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    ///int i; for(i=0;psIn[i]!='\0';i++) if(psIn[i]==psDlm[i]) return psIn; return psIn+i;
    int j=0;
    StringInput psIn = delimbuff.first;
    if(psDlm[0]=='\0') { *delimbuff.second=atof(psIn.c_str()); return psIn+strlen(psIn.c_str()); }
    for(int i=0;psIn[i]!='\0';i++) {
      if(psIn[i]==psDlm[j]) j++;
      else j=0;
      strTemp[i]=psIn[i];
      if(j==int(strlen(psDlm))) { strTemp[i+1-j]='\0'; *delimbuff.second=atof(strTemp.c_array()); return psIn+i+1;}
    }
    return NULL; //psIn;
  }
};
char*             StringInput::PS_EOS = strdup("<EOS>");
const StringInput StringInput::SI_EOS = StringInput::PS_EOS;
Array<char>       StringInput::strTemp ( 100 );


/*
template<class T>
pair<StringInput,T*> operator>> ( StringInput si, T& t ) {
  return pair<StringInput,T*>(si,&t);
}
*/

/*
template<class T>
class DelimiterBuffer {
 //private:
 public:
  char* psIn;
  T& tVar;
 public:
  DelimiterBuffer<T> ( char* ps, T& t ) : psIn(ps), tVar(t) { }
  //char* operator>> ( const char* psDlm ) {
  //  int j=0;
  //  for(int i=0;psIn[i]!='\0';i++) {
  //    if(psIn[i]==psDlm[j]) j++;
  //    if(j==strlen(psDlm)) { psIn[i+1-j]='\0'; psIn>>tVar; return psIn+i;}
  //  }
  //  return psIn;
  //}
};
*/


////////////////////////////////////////////////////////////////////////////////

class String : public Array<char> {
 private:
  static Array<char> strTemp;
 public:
  // Constructor / destructor methods...
  String ( )                         : Array<char>(1)          { operator[](0)='\0'; }
  String ( const String& str )       : Array<char>(str)        { }
  explicit String ( unsigned int i ) : Array<char>(i)          { operator[](0)='\0'; }
  explicit String ( const char* ps ) : Array<char>(strlen(ps)) { for(unsigned int i=0; ps[i]!='\0'; i++) Array<char>::add()=ps[i]; Array<char>::add()='\0'; }
  // Specification methods...
  String&        ensureCapacity ( unsigned int i )                 { return static_cast<String&> ( Array<char>::ensureCapacity(i) ); }
  String&        clear          ( )                                { Array<char>::clear(); operator[](0)='\0'; return *this; }
  char&          add            ( )                                { Array<char>::add()='\0'; return set(size()-2); }
  friend String& operator<<     ( String& str, double d )          { str.addReserve(20); uint32_t i=str.size();
                                                                     uint32_t j=sprintf(str.c_array()+i-1,"%g",d); assert(j<20);
                                                                     str[i+j-1]='\0'; return str; } //str[i+j]=str[i+j]; return str; }
  friend String& operator<<     ( String& str, const string& s )   { return str<<s.c_str(); }  // NOTE: SECOND ARG IS STDLIB STRING!
  friend String& operator<<     ( String& str, const String& s )   { return str<<s.c_array(); }
  friend String& operator<<     ( String& str, const char* ps )    { unsigned int j=str.size()-1; unsigned int i=0;
                                                                     for(i=0;ps[i]!='\0';i++) str[j+i]=ps[i];
                                                                     str[j+i]='\0'; return str; }
  Array<char*>&  split          ( Array<char*>& aps, const char* psDlm ) { aps.clear(); char* psT=NULL;
                                                                           for(int i=0;true;i++)
                                                                             { char* z=strtok_r( (0==i)?c_array():NULL, psDlm, &psT );
                                                                               if (!z) break;
                                                                               aps[i]=z; }
                                                                           return aps; }
  // Input / output methods...
  friend ostream& operator<<    ( ostream& os, const String& str ) { return os<<str.c_array(); }

  //friend explicit operator int ( const String& s ) { return atoi(s.c_array()); }

  friend pair<StringInput,String*> operator>> ( const StringInput ps, String& s ) { return pair<StringInput,String*>(ps,&s); }
  friend StringInput operator>> ( pair<StringInput,String*> delimbuff, const char* psDlm ) {
    if (StringInput(NULL)==delimbuff.first) return delimbuff.first;
    ////assert(*delimbuff.second<domain.getSize()); 
    int j=0;
    StringInput psIn = delimbuff.first;
    if(psDlm[0]=='\0') { *delimbuff.second=String(psIn.c_str()); return psIn+strlen(psIn.c_str()); }
    for(int i=0;psIn[i]!='\0';i++) {
      if(psIn[i]==psDlm[j]) j++;
      else j=0;
      strTemp[i]=psIn[i];
      if(j==int(strlen(psDlm))) { strTemp[i+1-j]='\0'; *delimbuff.second=String(strTemp.c_array()); return psIn+i+1;}
    }
    return NULL; //psIn;
  }
};
Array<char> String::strTemp ( 100 );




#endif //_NL_STRING__

