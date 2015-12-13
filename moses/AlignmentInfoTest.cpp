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

#include "AlignmentInfo.h"
#include "AlignmentInfoCollection.h"

using namespace Moses;
using namespace std;

BOOST_AUTO_TEST_SUITE(alignment_info)

typedef pair<size_t,size_t> IndexPair;
typedef set<pair<size_t,size_t> > IndexSet;

struct AlignmentInfoFixture {
  const AlignmentInfo* ai1;
  const AlignmentInfo* ai2;
  const AlignmentInfo* ai3;

  AlignmentInfoFixture() {
    AlignmentInfoCollection& collection = AlignmentInfoCollection::Instance();
    IndexSet aligns1,aligns2,aligns3;
    aligns1.insert(IndexPair(1,1));
    aligns1.insert(IndexPair(2,1));
    aligns2.insert(IndexPair(1,1));
    aligns2.insert(IndexPair(2,1));
    aligns3.insert(IndexPair(1,2));
    aligns3.insert(IndexPair(2,1));
    ai1 = collection.Add(aligns1);
    ai2 = collection.Add(aligns2);
    ai3 = collection.Add(aligns3);
  }

};

BOOST_FIXTURE_TEST_CASE(comparator, AlignmentInfoFixture)
{
  BOOST_CHECK(*ai1 == *ai2);
  BOOST_CHECK(*ai1 == *ai1);
  BOOST_CHECK(*ai2 == *ai2);
  BOOST_CHECK(*ai3 == *ai3);
  BOOST_CHECK(!(*ai2 == *ai3));
  BOOST_CHECK(!(*ai1 == *ai3));
}

BOOST_FIXTURE_TEST_CASE(hasher, AlignmentInfoFixture)
{
  //simple test that same objects give same hash
  AlignmentInfoHasher hash;
  BOOST_CHECK_EQUAL(hash(*ai1), hash(*ai2));
}

BOOST_AUTO_TEST_SUITE_END()
