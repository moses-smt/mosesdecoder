/*********************************
tercpp: an open-source Translation Edit Rate (TER) scorer tool for Machine Translation.

Copyright 2010-2013, Christophe Servan, LIUM, University of Le Mans, France
Contact: christophe.servan@lium.univ-lemans.fr

The tercpp tool and library are free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the licence, or
(at your option) any later version.

This program and library are distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**********************************/

#include "terAlignment.h"
using namespace std;
namespace TERCPPNS_TERCpp
{

terAlignment::terAlignment()
{
// 		vector<string> ref;
// 		vector<string> hyp;
// 		vector<string> aftershift;

  //   TERshift[] allshifts = null;

  numEdits=0;
  numWords=0;
//         bestRef="";

  numIns=0;
  numDel=0;
  numSub=0;
  numSft=0;
  numWsf=0;
  averageWords=0;

}
void terAlignment::set(terAlignment& l_terAlignment)
{
  numEdits=l_terAlignment.numEdits;
  numWords=l_terAlignment.numWords;
  bestRef=l_terAlignment.bestRef;
  numIns=l_terAlignment.numIns;
  numDel=l_terAlignment.numDel;
  numSub=l_terAlignment.numSub;
  numSft=l_terAlignment.numSft;
  numWsf=l_terAlignment.numWsf;
  averageWords=l_terAlignment.averageWords;
  ref=l_terAlignment.ref;
  hyp=l_terAlignment.hyp;
  aftershift=l_terAlignment.aftershift;
// 	allshifts=l_terAlignment.allshifts;
  hyp_int=l_terAlignment.hyp_int;
  aftershift_int=l_terAlignment.aftershift_int;
  alignment=l_terAlignment.alignment;
  allshifts=(*(new vector<terShift>((int)l_terAlignment.allshifts.size())));
  for (int l_i=0; l_i< (int)l_terAlignment.allshifts.size(); l_i++) {
    allshifts.at(l_i).set(l_terAlignment.allshifts.at(l_i));
  }

}
void terAlignment::set(terAlignment* l_terAlignment)
{
  numEdits=l_terAlignment->numEdits;
  numWords=l_terAlignment->numWords;
  bestRef=l_terAlignment->bestRef;
  numIns=l_terAlignment->numIns;
  numDel=l_terAlignment->numDel;
  numSub=l_terAlignment->numSub;
  numSft=l_terAlignment->numSft;
  numWsf=l_terAlignment->numWsf;
  averageWords=l_terAlignment->averageWords;
  ref=l_terAlignment->ref;
  hyp=l_terAlignment->hyp;
  aftershift=l_terAlignment->aftershift;
// 	allshifts=l_terAlignment->allshifts;
  hyp_int=l_terAlignment->hyp_int;
  aftershift_int=l_terAlignment->aftershift_int;
  alignment=l_terAlignment->alignment;
  allshifts=(*(new vector<terShift>((int)l_terAlignment->allshifts.size())));
  for (int l_i=0; l_i< (int)l_terAlignment->allshifts.size(); l_i++) {
    allshifts.at(l_i).set(l_terAlignment->allshifts.at(l_i));
  }

}

string terAlignment::toString()
{
  stringstream s;
  s.str ( "" );
  s << "Original Ref:   \t" << join ( " ", ref ) << endl;
  s << "Original Hyp:   \t" << join ( " ", hyp ) <<endl;
  s << "Hyp After Shift:\t" << join ( " ", aftershift );
// 	s << "Hyp After Shift: " << join ( " ", aftershift );
  s << endl;
// 		string s = "Original Ref: " + join(" ", ref) + 	"\nOriginal Hyp: " + join(" ", hyp) + "\nHyp After Shift: " + join(" ", aftershift);
  if ( ( int ) sizeof ( alignment ) >0 ) {
    s << "Alignment: (";
// 			s += "\nAlignment: (";
    for ( int i = 0; i < ( int ) ( alignment.size() ); i++ ) {
      s << alignment[i];
// 				s+=alignment[i];
    }
// 			s += ")";
    s << ")";
  }
  s << endl;
  if ( ( int ) allshifts.size() == 0 ) {
// 			s += "\nNumShifts: 0";
    s << "NumShifts: 0";
  } else {
// 			s += "\nNumShifts: " + (int)allshifts.size();
    s << "NumShifts: "<< ( int ) allshifts.size();
    for ( int i = 0; i < ( int ) allshifts.size(); i++ ) {
      s << endl << "  " ;
      s << ( ( terShift ) allshifts[i] ).toString();
// 				s += "\n  " + allshifts[i];
    }
  }
  s << endl << "Score: " << scoreAv() << " (" << numEdits << "/" << averageWords << ")";
// 		s += "\nScore: " + score() + " (" + numEdits + "/" + numWords + ")";
  return s.str();

}
string terAlignment::join ( string delim, vector<string> arr )
{
  if ( ( int ) arr.size() == 0 ) return "";
// 		if ((int)delim.compare("") == 0) delim = new String("");
// 		String s = new String("");
  stringstream s;
  s.str ( "" );
  for ( int i = 0; i < ( int ) arr.size(); i++ ) {
    if ( i == 0 ) {
      s << arr.at ( i );
    } else {
      s << delim << arr.at ( i );
    }
  }
  return s.str();
// 		return "";
}
double terAlignment::score()
{
  if ( ( numWords <= 0.0 ) && ( numEdits > 0.0 ) ) {
    return 1.0;
  }
  if ( numWords <= 0.0 ) {
    return 0.0;
  }
  return ( double ) numEdits / numWords;
}
double terAlignment::scoreAv()
{
  if ( ( averageWords <= 0.0 ) && ( numEdits > 0.0 ) ) {
    return 1.0;
  }
  if ( averageWords <= 0.0 ) {
    return 0.0;
  }
  return ( double ) numEdits / averageWords;
}

void terAlignment::scoreDetails()
{
  numIns = numDel = numSub = numWsf = numSft = 0;
  if((int)allshifts.size()>0) {
    for(int i = 0; i < (int)allshifts.size(); ++i) {
      numWsf += allshifts[i].size();
    }
    numSft = allshifts.size();
  }

  if((int)alignment.size()>0 ) {
    for(int i = 0; i < (int)alignment.size(); ++i) {
      switch (alignment[i]) {
      case 'S':
      case 'T':
        numSub++;
        break;
      case 'D':
        numDel++;
        break;
      case 'I':
        numIns++;
        break;
      }
    }
  }
  //	if(numEdits != numSft + numDel + numIns + numSub)
  //      System.out.println("** Error, unmatch edit erros " + numEdits +
  //                         " vs " + (numSft + numDel + numIns + numSub));
}
string terAlignment::printAlignments()
{
  stringstream to_return;
  for(int i = 0; i < (int)alignment.size(); ++i) {
    char alignInfo=alignment.at(i);
    if (alignInfo == 'A' ) {
      alignInfo='A';
    }

    if (i==0) {
      to_return << alignInfo;
    } else {
      to_return << " " << alignInfo;
    }
  }
  return to_return.str();
}
string terAlignment::printAllShifts()
{
  stringstream to_return;
  if ( ( int ) allshifts.size() == 0 ) {
// 			s += "\nNumShifts: 0";
    to_return << "NbrShifts: 0";
  } else {
// 			s += "\nNumShifts: " + (int)allshifts.size();
    to_return << "NbrShifts: "<< ( int ) allshifts.size();
    for ( int i = 0; i < ( int ) allshifts.size(); i++ ) {
      to_return << "\t" ;
      to_return << ( ( terShift ) allshifts[i] ).toString();
// 				s += "\n  " + allshifts[i];
    }
  }
  return to_return.str();
}

}
