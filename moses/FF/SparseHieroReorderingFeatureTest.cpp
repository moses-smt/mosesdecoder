/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2013- University of Edinburgh

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
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "SparseHieroReorderingFeature.h"

using namespace Moses;
using namespace std;

BOOST_AUTO_TEST_SUITE(shrf)

BOOST_AUTO_TEST_CASE(lexical_rule)
{
  SparseHieroReorderingFeature feature("name=shrf");

}

BOOST_AUTO_TEST_SUITE_END()
