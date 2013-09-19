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

#ifndef NL_IO_MACROS__
#define NL_IO_MACROS__

//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netdb.h>


#define NUM(c)                   ((c>='0' && c<='9'))
#define ALPHANUM(c)              ((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9'))
#define SPACE(c)                 (c==' ')
#define WHITESPACE(c)            (c==' ' || c=='\t' || c=='\n')

#define CONSUME_OPT(f,c,b,l)     c = (b) ? getc(f)+0*(l+=(c=='\n')?1:0) : c

#define CONSUME_ONE(f,c,b,s,l)   c = (b) ? getc(f)+0*(l+=(c=='\n')?1:0) : c+(0*fprintf(stderr,"\nERROR: %s in line %d (char=%c).\n\n",s,l,c))
#define CONSUME_ONE_STDIN(c,b,s,l) c = (b) ? getchar()+0*(l+=(c=='\n')?1:0) : c+(0*fprintf(stderr,"\nERROR: %s in line %d.\n\n",s,l))

#define CONSUME_ALL(f,c,b,l)     for ( ; b; c=getc(f)+0*(l+=(c=='\n')?1:0) )
#define CONSUME_ALL_STDIN(c,b,l) for ( ; b; c=getchar()+0*(l+=(c=='\n')?1:0) )

#define CONSUME_STR(f,c,b,s,i,l)      for ( i=0; (b) || false != (s[i++]='\0'); s[i++]=c, c=getc(f)+0*(l+=(c=='\n')?1:0) )
#define CONSUME_STR_SAFE(f,c,b,s,i,m,l) for ( i=0; i<m-1&&((b)||false!=(s[i++]='\0')); s[i++]=c, c=getc(f)+0*(l+=(c=='\n')?1:0) )
#define CONSUME_STR_STDIN(c,b,s,i,l)  for ( i=0; (b) || false != (s[i++]='\0'); s[i++]=c, c=getchar()+0*(l+=(c=='\n')?1:0) )

#define CONSUME_INT(f,c,i,l)     for ( i=0; (c>='0' && c<='9'); i=(i*10)+(c-'0'), c=getc(f)+0*(l+=(c=='\n')?1:0) )
#define CONSUME_INT_STDIN(c,i,l) for ( i=0; (c>='0' && c<='9'); i=(i*10)+(c-'0'), c=getchar()+0*(l+=(c=='\n')?1:0) )

#define CONSUME_DEC(f,c,i,j,l)   for ( j=1; (c>='0' && c<='9'); j*=10, i+=(c-'0')/j, c=getc(f)+0*(l+=(c=='\n')?1:0) )

#define CONSUME_HEX(f,c,i,l)     for ( i=0; (c>='0' && c<='9') || (c>='a' && c<='f'); i=(i*16)+((c<'a')?c-'0':c+10-'a'), c=getc(f)+0*(l+=(c=='\n')?1:0) )

#define CONSUME_ALL_SOCKET(f,c,b,l)     for ( char s[1]; b; c=((recv(f,&s[0],1,MSG_WAITALL)==1) ? s[0]+0*(l+=(c=='\n')?1:0) : '\0') )
#define CONSUME_STR_SOCKET(f,c,b,s,i,l) for ( i=0; (b) || ('\0'!=(s[i++]='\0')); s[i++]=c, c=((recv(f,&s[i],1,MSG_WAITALL)==1) ? s[i]+0*(l+=(c=='\n')?1:0) : (s[i]='\0')) )

//#define CONSUME_ALL_STRING(f,c,b,l)     for ( int ii=0; b && f[ii]!='\0'; c=f[ii]+0*(l+=(c=='\n')?1:0), ii++ )
//#define CONSUME_STR_STRING(f,c,b,s,i,l) for ( i=0; (b && f[i]!='\0') || false != (s[i++]='\0'); s[i++]=c, c=f[i]+0*(l+=(c=='\n')?1:0) )

#endif //_NL_IO_MACROS__
