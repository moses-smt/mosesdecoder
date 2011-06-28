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

#include <fstream>
#include <iostream>

#include <boost/archive/text_oarchive.hpp>

#include "FeatureVector.h"

using namespace Josiah;
using namespace std;

int main() {
  FVector fv;
  FName g3("L", "1");
  FName g4("L", "2");
  FName t1("T", "1");
  FName p2("P", "2");
  
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
  cerr << "fv.fvprod=" <<  inner_product(fv,fvprod) << endl;
  cerr << "fvprod.fv=" <<  inner_product(fvprod,fv) << endl;
  
  cerr << "fv * fv2 = " << (fv*fv2) << endl;
  cerr << "fv / fv2 = " << (fv/fv2) << endl;
  
  FVector fvp2 = fv + 2.0;
  cerr << "fv + 2 = " << fvp2 << endl;
  //cerr << "(fv+2)[" << g3 << "] = " << fvp2[g3] << " (fv+2)[" << g4 << "] = " << fvp2[g4] << " (fv+2)[" << t1 << "] = " << fvp2[t1] <<  endl;
  
  FVector fv2m1 = fv2 - 1.0;
  cerr << "(fv + 2) + (fv2 -1) = " << (fvp2 + fv2m1) << endl;
  cerr << "(fv + 2) - (fv2 -1) = " << (fvp2 - fv2m1) << endl;
  cerr << "(fv + 2) * (fv2 -1) = " << (fvp2 * fv2m1) << endl;
  cerr << "(fv + 2) / (fv2 -1) = " << (fvp2 / fv2m1) << endl;
  cerr << "max((fv + 2),(fv2 -1)) = " << fvmax(fvp2,fv2m1) << endl;
  
  cerr << "(fv + 2) + (fv2) = " << (fvp2 + fv2) << endl;
  cerr << "(fv + 2) - (fv2) = " << (fvp2 - fv2) << endl;
  cerr << "(fv + 2) * (fv2) = " << (fvp2 * fv2) << endl;
  cerr << "fv2 / (fv + 2) = " << (fv2 / fvp2) << endl;
  cerr << "max((fv + 2),(fv2)) = " << fvmax(fv2,fvp2) << endl;
  //fv2[g4] = 3.1; //error
  
  cerr << "fv2 . (fv + 2) = " << inner_product(fv2,fv+2) << endl;
  cerr << "(fv2-1) . (fv) = " << inner_product(fv2-1,fv) << endl;
  
  cerr << "(fv -1)[p2] = " << (fv -1)[p2] << endl;
  
  cerr << "fvp2 = " << fvp2 << endl;
  cerr << "++fvp2[g3] = " << ++fvp2[g3] << endl;
  //cerr << "fvp2[g3] = " << ++fvp2[g3] << endl;
  cerr << "fvp2 = " << fvp2 << endl;
  fvp2[p2] += 5;
  cerr << "fvp2 = " << fvp2 << endl;

  
  FVector loaded;
  loaded.load("weights.txt");
  cerr << "loaded=" << loaded << endl;
  

  return 0;
}
