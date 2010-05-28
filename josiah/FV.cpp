/*
     Moses - factored phrase-based language decoder
     Copyright (C) 2010 University of Edinburgh


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include <iostream>

#include "FeatureVector.h"

using namespace Josiah;
using namespace std;

int main() {
  FVector fv;
  FName g3("LM", "3gram");
  FName g4("LM", "4gram");
  FName t1("TM", "pef");
  
  fv[g3] = 2.0;
  fv[g4] = 1.3;
  cerr << "fv=" << fv << endl;
  

  
  FVector fv2;
  fv2[g3] = 1.5;
  fv2[t1] = 3.0;
  
  FVector fvsum = fv + fv2;
  FVector fvdiff = fv - fv2;
  FVector fvprod = fv * 1.4;
  FVector fvdiv = fv  / 4.0;
  
  cerr << "fv2=" << fv2 << endl;
  cerr << "fvsum=" << fvsum << endl;
  cerr << "fvdiff=" << fvdiff << endl;
  cerr << "fvprod=" << fvprod << endl;
  cerr << "fvdiv=" << fvdiv << endl;
  cerr << "fv.fvprod=" <<  (fv*fvprod) << endl;
  cerr << "fvprod.fv=" <<  (fvprod*fv) << endl;
  //fv2[g4] = 3.1; //error
  

  return 0;
}