#include "MiraFeatureVector.h"
#include "MiraWeightVector.h"

#define BOOST_TEST_MODULE MiraFeatureVector
#include <boost/test/unit_test.hpp>

using namespace MosesTuning;

/* Note that the conversion to and from SparseVector needs to know
how many of the features are really "dense". This is because in hg mira
all features (sparse and dense) are to get rolled in to SparseVector
*/

BOOST_AUTO_TEST_CASE(from_sparse)
{
  SparseVector sp;
  sp.set("dense0", 0.2);
  sp.set("dense1", 0.3);
  sp.set("sparse0", 0.7);
  sp.set("sparse1", 0.9);
  sp.set("sparse2", 0.1);

  MiraFeatureVector mfv(sp,2);
  BOOST_CHECK_EQUAL(mfv.size(),5);

  BOOST_CHECK_EQUAL(mfv.feat(0),0);
  BOOST_CHECK_EQUAL(mfv.feat(1),1);
  BOOST_CHECK_EQUAL(mfv.feat(2),4);
  BOOST_CHECK_EQUAL(mfv.feat(3),5);
  BOOST_CHECK_EQUAL(mfv.feat(4),6);

  BOOST_CHECK_CLOSE(mfv.val(0), 0.2,1e-5);
  BOOST_CHECK_CLOSE(mfv.val(1), 0.3,1e-5);
  BOOST_CHECK_CLOSE(mfv.val(2), 0.7,1e-5);
  BOOST_CHECK_CLOSE(mfv.val(3), 0.9,1e-5);
  BOOST_CHECK_CLOSE(mfv.val(4), 0.1,1e-5);

  MiraWeightVector mwv;
  mwv.update(mfv,1.0);
  SparseVector sp2;
  mwv.ToSparse(&sp2,2);

  //check we get back what we started with
  BOOST_CHECK_CLOSE(sp2.get("dense0"), 0.2,1e-5);
  BOOST_CHECK_CLOSE(sp2.get("dense1"), 0.3,1e-5);
  BOOST_CHECK_CLOSE(sp2.get("sparse0"), 0.7,1e-5);
  BOOST_CHECK_CLOSE(sp2.get("sparse1"), 0.9,1e-5);
  BOOST_CHECK_CLOSE(sp2.get("sparse2"), 0.1,1e-5);

}
