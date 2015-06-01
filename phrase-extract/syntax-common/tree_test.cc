#include "tree.h"

#define BOOST_TEST_MODULE TreeTest
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>

namespace MosesTraining {
namespace Syntax {
namespace {

// Test Tree<>::PreOrderIterator with a trivial, single-node tree.
BOOST_AUTO_TEST_CASE(pre_order_1) {
  boost::scoped_ptr<Tree<int> > root(new Tree<int>(123));
  Tree<int>::PreOrderIterator p(*root);
  BOOST_REQUIRE(p != Tree<int>::PreOrderIterator());
  BOOST_REQUIRE(p->value() == 123);
  ++p;
  BOOST_REQUIRE(p == Tree<int>::PreOrderIterator());
}

// Test Tree<>::PreOrderIterator on this tree: (1 (2 3) (4) (5 6 (7 8)))
BOOST_AUTO_TEST_CASE(pre_order_2) {
  boost::scoped_ptr<Tree<int> > root(new Tree<int>(1));
  root->children().push_back(new Tree<int>(2));
  root->children()[0]->children().push_back(new Tree<int>(3));
  root->children().push_back(new Tree<int>(4));
  root->children().push_back(new Tree<int>(5));
  root->children()[2]->children().push_back(new Tree<int>(6));
  root->children()[2]->children().push_back(new Tree<int>(7));
  root->children()[2]->children()[1]->children().push_back(new Tree<int>(8));
  root->SetParents();

  Tree<int>::PreOrderIterator p(*root);
  Tree<int>::PreOrderIterator end;

  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 1);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 2);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 3);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 4);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 5);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 6);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 7);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 8);
  ++p;
  BOOST_REQUIRE(p == end);
}

// Test Tree<>::ConstPreOrderIterator on this tree: (1 (2 (3 (4 (5) (6)))) (7))
BOOST_AUTO_TEST_CASE(const_pre_order_1) {
  boost::scoped_ptr<Tree<int> > root(new Tree<int>(1));
  root->children().push_back(new Tree<int>(2));
  root->children()[0]->children().push_back(new Tree<int>(3));
  root->children()[0]->children()[0]->children().push_back(new Tree<int>(4));
  root->children()[0]->children()[0]->children()[0]->children().push_back(
      new Tree<int>(5));
  root->children()[0]->children()[0]->children()[0]->children().push_back(
      new Tree<int>(6));
  root->children().push_back(new Tree<int>(7));
  root->SetParents();

  Tree<int>::ConstPreOrderIterator p(*root);
  Tree<int>::ConstPreOrderIterator end;

  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 1);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 2);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 3);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 4);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 5);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 6);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 7);
  ++p;
  BOOST_REQUIRE(p == end);
}

// Test Tree<>::LeafIterator with a trivial, single-node tree.
BOOST_AUTO_TEST_CASE(leaf_1) {
  boost::scoped_ptr<Tree<int> > root(new Tree<int>(123));
  Tree<int>::LeafIterator p(*root);
  BOOST_REQUIRE(p != Tree<int>::LeafIterator());
  BOOST_REQUIRE(p->value() == 123);
  ++p;
  BOOST_REQUIRE(p == Tree<int>::LeafIterator());
}

// Test Tree<>::LeafIterator on this tree: (1 (2 3) (4) (5 6 (7 8)))
BOOST_AUTO_TEST_CASE(leaf_2) {
  boost::scoped_ptr<Tree<int> > root(new Tree<int>(1));
  root->children().push_back(new Tree<int>(2));
  root->children()[0]->children().push_back(new Tree<int>(3));
  root->children().push_back(new Tree<int>(4));
  root->children().push_back(new Tree<int>(5));
  root->children()[2]->children().push_back(new Tree<int>(6));
  root->children()[2]->children().push_back(new Tree<int>(7));
  root->children()[2]->children()[1]->children().push_back(new Tree<int>(8));
  root->SetParents();

  Tree<int>::LeafIterator p(*root);
  Tree<int>::LeafIterator end;

  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 3);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 4);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 6);
  ++p;
  BOOST_REQUIRE(p != end);
  BOOST_REQUIRE(p->value() == 8);
  ++p;
  BOOST_REQUIRE(p == end);
}

}  // namespace
}  // namespace Syntax
}  // namespace MosesTraining
