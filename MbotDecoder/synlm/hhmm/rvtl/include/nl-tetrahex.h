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

/***********************************************
 * nl-tetrahex.h 
 * a little header with some base conversion stuff
 * so that we can represent base 16, 32 or 64 with
 * one character.
 * 0, 1, 2, ..., 9, A, B, ..., Y, Z, a, b, ..., z, {, |
 * 0  1  2       9  10 11      34 35 36 37      61 62 63
 ***********************************************/

#include <cassert>

#ifndef NL_TETRAHEX
#define NL_TETRAHEX

char intToTetraHex(int i) {
   assert(i < 64 && i >= 0);
   if(i < 10) return (i + '0');
   if(i > 35) return (i + 'a' - 36);
   return (i + 'A' - 10);
}

int tetraHexToInt(char c) {
   if(c < '0')
      fprintf(stderr, "Bad c: %d\n",(int)c);
   assert(c >= '0');
   if(c - '0' < 10) return (int)(c-'0');
   assert(c >= 'A');
   if(c - 'A' < 26) return (int)(c-'A'+10);
   assert(c >= 'a' && c < '}');
   return (int)(c-'a'+36);
}

#endif
