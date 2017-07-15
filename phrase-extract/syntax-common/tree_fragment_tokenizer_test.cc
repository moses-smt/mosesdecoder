#include "tree_fragment_tokenizer.h"

#define BOOST_TEST_MODULE TreeTest
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>

namespace MosesTraining {
namespace Syntax {
namespace {

BOOST_AUTO_TEST_CASE(tokenize_empty) {
  const std::string fragment = "";
  std::vector<TreeFragmentToken> tokens;
  for (TreeFragmentTokenizer p(fragment); p != TreeFragmentTokenizer(); ++p) {
    tokens.push_back(*p);
  }
  BOOST_REQUIRE(tokens.empty());
}

BOOST_AUTO_TEST_CASE(tokenize_space) {
  const std::string fragment = "  [  weasel weasel  ] [] ] wea[sel";
  std::vector<TreeFragmentToken> tokens;
  for (TreeFragmentTokenizer p(fragment); p != TreeFragmentTokenizer(); ++p) {
    tokens.push_back(*p);
  }
  BOOST_REQUIRE(tokens.size() == 10);
  BOOST_REQUIRE(tokens[0].type == TreeFragmentToken_LSB);
  BOOST_REQUIRE(tokens[0].value == "[");
  BOOST_REQUIRE(tokens[1].type == TreeFragmentToken_WORD);
  BOOST_REQUIRE(tokens[1].value == "weasel");
  BOOST_REQUIRE(tokens[2].type == TreeFragmentToken_WORD);
  BOOST_REQUIRE(tokens[2].value == "weasel");
  BOOST_REQUIRE(tokens[3].type == TreeFragmentToken_RSB);
  BOOST_REQUIRE(tokens[3].value == "]");
  BOOST_REQUIRE(tokens[4].type == TreeFragmentToken_LSB);
  BOOST_REQUIRE(tokens[4].value == "[");
  BOOST_REQUIRE(tokens[5].type == TreeFragmentToken_RSB);
  BOOST_REQUIRE(tokens[5].value == "]");
  BOOST_REQUIRE(tokens[6].type == TreeFragmentToken_RSB);
  BOOST_REQUIRE(tokens[6].value == "]");
  BOOST_REQUIRE(tokens[7].type == TreeFragmentToken_WORD);
  BOOST_REQUIRE(tokens[7].value == "wea");
  BOOST_REQUIRE(tokens[8].type == TreeFragmentToken_LSB);
  BOOST_REQUIRE(tokens[8].value == "[");
  BOOST_REQUIRE(tokens[9].type == TreeFragmentToken_WORD);
  BOOST_REQUIRE(tokens[9].value == "sel");
}

BOOST_AUTO_TEST_CASE(tokenize_fragment) {
  const std::string fragment = "[S [NP [NN weasels]] [VP]]";
  std::vector<TreeFragmentToken> tokens;
  for (TreeFragmentTokenizer p(fragment); p != TreeFragmentTokenizer(); ++p) {
    tokens.push_back(*p);
  }
  BOOST_REQUIRE(tokens.size() == 13);
  BOOST_REQUIRE(tokens[0].type == TreeFragmentToken_LSB);
  BOOST_REQUIRE(tokens[1].type == TreeFragmentToken_WORD);
  BOOST_REQUIRE(tokens[2].type == TreeFragmentToken_LSB);
  BOOST_REQUIRE(tokens[3].type == TreeFragmentToken_WORD);
  BOOST_REQUIRE(tokens[4].type == TreeFragmentToken_LSB);
  BOOST_REQUIRE(tokens[5].type == TreeFragmentToken_WORD);
  BOOST_REQUIRE(tokens[6].type == TreeFragmentToken_WORD);
  BOOST_REQUIRE(tokens[7].type == TreeFragmentToken_RSB);
  BOOST_REQUIRE(tokens[8].type == TreeFragmentToken_RSB);
  BOOST_REQUIRE(tokens[9].type == TreeFragmentToken_LSB);
  BOOST_REQUIRE(tokens[10].type == TreeFragmentToken_WORD);
  BOOST_REQUIRE(tokens[11].type == TreeFragmentToken_RSB);
  BOOST_REQUIRE(tokens[12].type == TreeFragmentToken_RSB);
}

}  // namespace
}  // namespace Syntax
}  // namespace MosesTraining
