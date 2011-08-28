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

BOOST_AUTO_TEST_CASE(core) 
{
  FVector f1(2);
  f1[0] = 1.3;
  f1[1] = -1.9;
  BOOST_CHECK_CLOSE(f1[0],1.3,TOL);
  BOOST_CHECK_CLOSE(f1[1],-1.9,TOL);
  f1[1] = 0.1;
  BOOST_CHECK_CLOSE(f1[1],0.1,TOL);
  
  BOOST_CHECK_EQUAL(f1.size(),2);

  f1[FName("a")] = 1.2;
  BOOST_CHECK_EQUAL(f1.size(),3);
}

BOOST_AUTO_TEST_CASE(core_arith)
{
  FVector f1(2);
  FVector f2(2);
  FName n1("a");
  FName n2("b");
  f1[0] = 1.1; f1[1] = 0.25; f1[n1] = 3.6; f1[n2] = -1.5;
  f2[0] = 0.5; f2[1] = -0.1; f2[n1] = 1;

  //vector ops
  FVector sum = f1+f2;
  FVector diff = f1-f2;
  FVector prod = f1*f2;
  FVector quot = f2/f1;

  BOOST_CHECK_CLOSE((FValue)sum[0], 1.6 , TOL);
  BOOST_CHECK_CLOSE((FValue)sum[1], 0.15 , TOL);
  BOOST_CHECK_CLOSE((FValue)sum[n1], 4.6  , TOL);
  BOOST_CHECK_CLOSE((FValue)sum[n2], -1.5 , TOL);

  BOOST_CHECK_CLOSE((FValue)diff[0], 0.6 , TOL);
  BOOST_CHECK_CLOSE((FValue)diff[1], 0.35 , TOL);
  BOOST_CHECK_CLOSE((FValue)diff[n1], 2.6  , TOL);
  BOOST_CHECK_CLOSE((FValue)diff[n2], -1.5 , TOL);

  BOOST_CHECK_CLOSE((FValue)prod[0], 0.55 , TOL);
  BOOST_CHECK_CLOSE((FValue)prod[1], -0.025 , TOL);
  BOOST_CHECK_CLOSE((FValue)prod[n1], 3.6  , TOL);
  BOOST_CHECK_CLOSE((FValue)prod[n2], 0 , TOL);

  BOOST_CHECK_CLOSE((FValue)quot[0], 0.4545454545 , TOL);
  BOOST_CHECK_CLOSE((FValue)quot[1], -0.4 , TOL);
  BOOST_CHECK_CLOSE((FValue)quot[n1], 0.277777777  , TOL);
  BOOST_CHECK_CLOSE((FValue)quot[n2], 0 , TOL);
}

BOOST_AUTO_TEST_CASE(core_scalar)
{
  FVector f1(3);
  FName n1("a");
  f1[0] = 1.5; f1[1] = 2.1; f1[2] = 4; f1[n1] = -0.5;

  FVector prod = f1*2;
  FVector quot = f1/5;

  BOOST_CHECK_CLOSE((FValue)prod[0], 3 , TOL);
  BOOST_CHECK_CLOSE((FValue)prod[1], 4.2 , TOL);
  BOOST_CHECK_CLOSE((FValue)prod[2], 8 , TOL);
  BOOST_CHECK_CLOSE((FValue)prod[n1],-1  , TOL);

  BOOST_CHECK_CLOSE((FValue)quot[0], 0.3 , TOL);
  BOOST_CHECK_CLOSE((FValue)quot[1], 0.42 , TOL);
  BOOST_CHECK_CLOSE((FValue)quot[2], 0.8 , TOL);
  BOOST_CHECK_CLOSE((FValue)quot[n1],-0.1  , TOL);

}

BOOST_AUTO_TEST_CASE(core_max) 
{
  FVector f1(2);
  FVector f2(2);
  FName n1("a");
  FName n2("b");
  FName n3("c");
  f1[0] = 1.1; f1[1] = -0.1; ; f1[n2] = -1.5; f1[n3] = 2.2;
  f2[0] = 0.5; f2[1] = 0.25; f2[n1] = 1; f2[n3] = 2.4;

  FVector m = fvmax(f1,f2);

  BOOST_CHECK_CLOSE((FValue)m[0], 1.1 , TOL);
  BOOST_CHECK_CLOSE((FValue)m[1], 0.25 , TOL);
  BOOST_CHECK_CLOSE((FValue)m[n1], 1 , TOL);
  BOOST_CHECK_CLOSE((FValue)m[n2],0  , TOL);
  BOOST_CHECK_CLOSE((FValue)m[n3],2.4  , TOL);

}

BOOST_AUTO_TEST_CASE(l1norm) 
{
  FVector f1(3);
  FName n1("a");
  f1[0] = 1.5; f1[1] = 2.1; f1[2] = 4; f1[n1] = -0.5;
  FValue n = f1.l1norm();
  BOOST_CHECK_CLOSE((FValue)n, abs(1.5)+abs(2.1)+abs(4)+abs(-0.5), TOL);
}


BOOST_AUTO_TEST_CASE(sum) 
{
  FVector f1(3);
  FName n1("a");
  FName n2("b");
  f1[0] = 1.5; f1[1] = 2.1; f1[2] = 4; f1[n1] = -0.5; f1[n2] = 2.7;
  FValue n = f1.sum();
  BOOST_CHECK_CLOSE((FValue)n, 1.5+2.1+4-0.5+2.7, TOL);
}

BOOST_AUTO_TEST_CASE(l2norm) 
{
  FVector f1(3);
  FName n1("a");
  f1[0] = 1.5; f1[1] = 2.1; f1[2] = 4; f1[n1] = -0.5;
  FValue n = f1.l2norm();
  BOOST_CHECK_CLOSE((FValue)n, sqrt((1.5*1.5)+(2.1*2.1)+(4*4)+(-0.5*-0.5)), TOL);
}

BOOST_AUTO_TEST_CASE(ip)
{
  FVector f1(2);
  FVector f2(2);
  FName n1("a");
  FName n2("b");
  FName n3("c");
  f1[0] = 1.1; f1[1] = -0.1; ; f1[n2] = -1.5; f1[n3] = 2.2;
  f2[0] = 0.5; f2[1] = 0.25; f2[n1] = 1; f2[n3] = 2.4;
  FValue p1 = inner_product(f1,f2);
  FValue p2 = inner_product(f2,f1);
  BOOST_CHECK_CLOSE(p1,p2,TOL);
  BOOST_CHECK_CLOSE((FValue)p1, 1.1*0.5 + -0.1*0.25 + 2.2*2.4, TOL);
}


BOOST_AUTO_TEST_SUITE_END()

