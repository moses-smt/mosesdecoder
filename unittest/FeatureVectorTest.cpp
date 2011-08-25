/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010- University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <boost/test/unit_test.hpp>

#include "FeatureVector.h"

using namespace Moses;
using namespace std;

static const float TOL = 0.00001;

BOOST_AUTO_TEST_SUITE(fv)

BOOST_AUTO_TEST_CASE(vector_sum_diff) 
{
  FVector f1,f2;
  FName n1("a");
  FName n2("b");
  FName n3("c");
  FName n4("d");
  f1[n1] = 1.2; f1[n2] = 1.4; f1[n3] = -0.1;
  f2[n1] = 0.01; f2[n3] = 5.6; f2[n4] = 0.6;
  FVector sum = f1 + f2;
  FVector diff = f1 - f2;
  BOOST_CHECK_CLOSE((FValue)sum[n1], 1.21, TOL); 
  BOOST_CHECK_CLOSE((FValue)sum[n2], 1.4, TOL); 
  BOOST_CHECK_CLOSE((FValue)sum[n3], 5.5, TOL); 
  BOOST_CHECK_CLOSE((FValue)sum[n4], 0.6, TOL); 
  BOOST_CHECK_CLOSE((FValue)diff[n1], 1.19, TOL); 
  BOOST_CHECK_CLOSE((FValue)diff[n2], 1.4, TOL); 
  BOOST_CHECK_CLOSE((FValue)diff[n3], -5.7, TOL); 
  BOOST_CHECK_CLOSE((FValue)diff[n4], -0.6, TOL); 
}


BOOST_AUTO_TEST_CASE(scalar) 
{
  FVector f1,f2;
  FName n1("a");
  FName n2("b");
  FName n3("c");
  FName n4("d");
  f1[n1] = 0.2; f1[n2] = 9.178; f1[n3] = -0.1;
  f2[n1] = 0.01; f2[n3] = 5.6; f2[n4] = 0.6;
  FVector prod1 = f1 * 2;
  FVector prod2 = f1 * -0.1;
  FVector quot = f2 / 2;
  BOOST_CHECK_CLOSE((FValue)prod1[n1], 0.4, TOL);
  BOOST_CHECK_CLOSE((FValue)prod1[n2], 18.356, TOL);
  BOOST_CHECK_CLOSE((FValue)prod1[n3], -0.2, TOL);

  BOOST_CHECK_CLOSE((FValue)prod2[n1], -0.02, TOL);
  BOOST_CHECK_CLOSE((FValue)prod2[n2], -0.9178, TOL);
  BOOST_CHECK_CLOSE((FValue)prod2[n3], 0.01, TOL);

  BOOST_CHECK_CLOSE((FValue)quot[n1], 0.005, TOL);
  BOOST_CHECK_CLOSE((FValue)quot[n2], 0, TOL);
  BOOST_CHECK_CLOSE((FValue)quot[n3], 2.8, TOL);
  BOOST_CHECK_CLOSE((FValue)quot[n4], 0.3, TOL);
}

BOOST_AUTO_TEST_CASE(inc) 
{
  FVector f1;
  FName n1("a");
  FName n2("b");
  f1[n1] = 2.3; f1[n2] = -0.4;
  f1[n1]+=2;
  BOOST_CHECK_CLOSE((FValue)f1[n1], 4.3, TOL);
  BOOST_CHECK_CLOSE((FValue)f1[n2], -0.4, TOL);

  FValue res = ++f1[n2];
  BOOST_CHECK_CLOSE(res,0.6, TOL);
  BOOST_CHECK_CLOSE((FValue)f1[n1], 4.3, TOL);
  BOOST_CHECK_CLOSE((FValue)f1[n2], 0.6, TOL);
}

BOOST_AUTO_TEST_CASE(vector_mult)
{
  FVector f1,f2;
  FName n1("a");
  FName n2("b");
  FName n3("c");
  FName n4("d");
  f1[n1] = 0.2; f1[n2] = 9.178;  f1[n3] = -0.1;
  f2[n1] = 0.01; f2[n2] = 5.6; f2[n3] = 1; f2[n4] = 0.6;
  FVector prod = f1 * f2;
  FVector quot = f1/f2;
  BOOST_CHECK_CLOSE((FValue)prod[n1], 0.002, TOL);
  BOOST_CHECK_CLOSE((FValue)prod[n2], 51.3968, TOL);
  BOOST_CHECK_CLOSE((FValue)prod[n3], -0.1, TOL);
  BOOST_CHECK_CLOSE((FValue)prod[n4], 0, TOL);

  BOOST_CHECK_CLOSE((FValue)quot[n1], 20, TOL);
  BOOST_CHECK_CLOSE((FValue)quot[n2], 1.63892865, TOL);
  BOOST_CHECK_CLOSE((FValue)quot[n3], -0.1, TOL);
  BOOST_CHECK_CLOSE((FValue)quot[n4], 0, TOL);
}

BOOST_AUTO_TEST_SUITE_END()

