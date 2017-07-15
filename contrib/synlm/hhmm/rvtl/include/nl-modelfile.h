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

#ifndef _NL_MODEL_FILE__
#define _NL_MODEL_FILE__

#include "nl-string.h"
#include "nl-iomacros.h"

#include <netinet/in.h>

static bool OUTPUT_QUIET = false;

///////////////////////////////////////////////////////////////////////////////

void processModelFilePtr ( FILE* pf, bool rF(Array<char*>&) ) {
  int i=0; int numFields=0; int c=' '; int line=1;
  CONSUME_ALL(pf,c,WHITESPACE(c),line);                           // Get to first record
  while ( c!=EOF ) {                                              // For each record
    if ( c=='#' ) CONSUME_ALL(pf, c, c!='\n' && c!='\0', line ) ; //   If comment, consume
    else {                                                        //   If no comment,
      Array<char*> aps(100);
      String       psBuff(1000);
      CONSUME_STR ( pf, c, (c!='\n' && c!='\0'), psBuff, i, line );

      char* psT=NULL;
      for(int i=0;true;i++) {
        char* z = strtok_r ( (0==i)?psBuff.c_array():NULL, " :=", &psT );
        if (!z) break;
        aps[i]=z;
      }

      if ( !rF(aps) )                                             //     Try to process fields, else complain
        fprintf( stderr, "\nERROR: %d %d-arg %s in line %d\n\n", numFields, aps.size(), aps[0], line);
    }
    CONSUME_ALL(pf,c,WHITESPACE(c),line);                         //   Consume whitespace
  }
}

///////////////////////////////////////////////////////////////////////////////

void processModelFile ( const char* ps, bool rF(Array<char*>&) ) {
  FILE* pf;
  if(!OUTPUT_QUIET) fprintf ( stderr, "Reading model file %s...\n", ps ) ;
  if ( NULL == (pf=fopen(ps,"r")) )                               // Complain if file not found
    fprintf ( stderr, "\nERROR: file %s could not be opened.\n\n", ps ) ;
  processModelFilePtr ( pf, rF );
  fclose(pf);
  if(!OUTPUT_QUIET) fprintf ( stderr, "Model file %s loaded.\n", ps ) ;
}

///////////////////////////////////////////////////////////////////////////////

void processModelSocket ( const int tSockfd, int& c, bool rF(Array<char*>&) ) {
  int i=0; int numFields=0; int line=1;
  CONSUME_ALL_SOCKET(tSockfd,c,WHITESPACE(c),line);                                          // Get to first record
  while ( c!='\0' && c!='\5' ) {                                                             // For each record
    if ( c=='#' ) CONSUME_ALL_SOCKET(tSockfd, c, (c!='\n' && c!='\0' && c!='\5'), line ) ;   //   If comment, consume
    else {                                                                                   //   If no comment,
      Array<char*> aps(100);
      String       psBuff(1000);
      CONSUME_STR_SOCKET ( tSockfd, c, (c!='\n' && c!='\0' && c!='\5'), psBuff, i, line );
      ////cerr<<"|"<<psBuff.c_array()<<"|"<<endl;

      char* psT=NULL;
      for(int i=0;true;i++) {
        char* z = strtok_r ( (0==i)?psBuff.c_array():NULL, " :=", &psT );
        if (!z) break;
        aps[i]=z;
      }

      if ( !rF(aps) )                                                     //     Try to process fields, else complain
        fprintf( stderr, "\nERROR: %d-arg %s in line %d\n\n", numFields, aps[0], line);
    }
    CONSUME_ALL_SOCKET(tSockfd,c,WHITESPACE(c),line);                     //   Consume whitespace
  }
}

void processModelSocket ( const int tSockfd, bool rF(Array<char*>&) ) {
  int c=' ';
  processModelSocket ( tSockfd, c, rF );
}

///////////////////////////////////////////////////////////////////////////////

/*
void processModelString ( String& sBuff, bool rF(Array<char*>&) ) {
  if ('#'!=sBuff[0]) {
    Array<char*> aps(100);
    char* psT=NULL;
    for(int i=0;true;i++) {
      char* z = strtok_r ( (0==i)?sBuff.c_array():NULL, " :=", &psT );
      if (!z) break;
      aps[i]=z;
    }
    if ( !rF(aps) )                                                     //     Try to process fields, else complain
      fprintf( stderr, "\nERROR: %d-arg %s in line %d\n\n", numFields, aps[0], line);
  }
}
*/

///////////////////////////////////////////////////////////////////////////////

#endif //_NL_MODEL_FILE__


