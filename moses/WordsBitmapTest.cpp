/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2015- University of Edinburgh

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

#include <vector>

#include "WordsBitmap.h"

using namespace Moses;
using namespace std;

BOOST_AUTO_TEST_SUITE(bitmap)

BOOST_AUTO_TEST_CASE(initialise)
{
  WordsBitmap wbm(5);
  BOOST_CHECK_EQUAL(wbm.GetSize(),5);
  for (size_t i = 0; i < 5; ++i) {
    BOOST_CHECK_EQUAL(wbm.GetValue(i),false);
  }

  vector<bool> bitvec(10);
  bitvec[2] = true;
  bitvec[3] = true;
  bitvec[7] = true;

  WordsBitmap wbm2(7,bitvec);
  BOOST_CHECK_EQUAL(wbm2.GetSize(),7);
  for (size_t i = 0; i < 7; ++i) {
    if (i  != 2 && i != 3) {
      BOOST_CHECK_EQUAL(wbm2.GetValue(i), false);
    } else {
      BOOST_CHECK_EQUAL(wbm2.GetValue(i), true);
    }
  }


}

BOOST_AUTO_TEST_CASE(getset)
{
  WordsBitmap wbm(6);
  wbm.SetValue(1,true);
  BOOST_CHECK_EQUAL(wbm.GetValue(1),true);
  BOOST_CHECK_EQUAL(wbm.GetValue(2),false);
  wbm.SetValue(2,true);
  BOOST_CHECK_EQUAL(wbm.GetValue(2),true);

  wbm.SetValue(1,3,true);
  BOOST_CHECK_EQUAL(wbm.GetValue(1),true);
  BOOST_CHECK_EQUAL(wbm.GetValue(2),true);
  BOOST_CHECK_EQUAL(wbm.GetValue(3),true);
  BOOST_CHECK_EQUAL(wbm.GetValue(4),false);

  WordsBitmap wbm2(6);
  WordsRange wr(2,4);
  wbm2.SetValue(wr,true);
  BOOST_CHECK_EQUAL(wbm2.GetValue(2),true);
  BOOST_CHECK_EQUAL(wbm2.GetValue(3),true);
  BOOST_CHECK_EQUAL(wbm2.GetValue(4),true);

  wbm2.SetValue(wr,false);
  BOOST_CHECK_EQUAL(wbm2.GetValue(2),false);
  BOOST_CHECK_EQUAL(wbm2.GetValue(3),false);
  BOOST_CHECK_EQUAL(wbm2.GetValue(4),false);

}

BOOST_AUTO_TEST_CASE(covered)
{
  WordsBitmap wbm(10);
  BOOST_CHECK_EQUAL(wbm.GetNumWordsCovered(), 0);
  wbm.SetValue(1,true);
  wbm.SetValue(2,true);
  BOOST_CHECK_EQUAL(wbm.GetNumWordsCovered(), 2);
  wbm.SetValue(3,true);
  wbm.SetValue(7,true);
  BOOST_CHECK_EQUAL(wbm.GetNumWordsCovered(), 4);
  wbm.SetValue(2,true);
  BOOST_CHECK_EQUAL(wbm.GetNumWordsCovered(), 4);
  wbm.SetValue(2,false);
  BOOST_CHECK_EQUAL(wbm.GetNumWordsCovered(), 3);
}

BOOST_AUTO_TEST_CASE(positions)
{
  WordsBitmap wbm(10);
  wbm.SetValue(0,true);
  wbm.SetValue(1,true);
  wbm.SetValue(3,true);
  wbm.SetValue(7,true);
  BOOST_CHECK_EQUAL(wbm.GetFirstGapPos(), 2);
  BOOST_CHECK_EQUAL(wbm.GetLastGapPos(), 9);
  BOOST_CHECK_EQUAL(wbm.GetLastPos(), 7);

  WordsBitmap wbm2(2);
  wbm2.SetValue(0,true);
  wbm2.SetValue(1,true);
  BOOST_CHECK_EQUAL(wbm2.GetFirstGapPos(), NOT_FOUND);

  WordsBitmap wbm3(5);
  BOOST_CHECK_EQUAL(wbm3.GetFirstGapPos(), 0);
  BOOST_CHECK_EQUAL(wbm3.GetLastGapPos(), 4);
  BOOST_CHECK_EQUAL(wbm3.GetLastPos(), NOT_FOUND);


}

BOOST_AUTO_TEST_SUITE_END()

