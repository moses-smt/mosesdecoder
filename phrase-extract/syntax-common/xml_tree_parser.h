#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "SyntaxNodeCollection.h"
#include "SyntaxTree.h"

namespace MosesTraining {
namespace Syntax {

/** Parses string representations of parse trees in Moses' XML format and
 *  converts them to SyntaxTree objects.
 *
 *  This is a thin wrapper around the ProcessAndStripXMLTags function.  After
 *  calling Parse(), the output from the ProcessAndStripXMLTags call (the
 *  sentence, node collection, label set, and top label set) are available via
 *  accessors.
 */
class XmlTreeParser {
 public:
  //! Parse a single sentence and return a SyntaxTree (with words attached).
  std::auto_ptr<SyntaxTree> Parse(const std::string &, bool unescape=false);

  //! Get the sentence string (as returned by ProcessAndStripXMLTags).
  const std::string &sentence() const { return sentence_; }

  //! Get the sentence as a vector of words.
  const std::vector<std::string> &words() const { return words_; }

  //! Get the node collection (as returned by ProcessAndStripXMLTags).
  const SyntaxNodeCollection &node_collection() const {
    return node_collection_;
  }

  //! Get the label set (as returned by ProcessAndStripXMLTags).
  const std::set<std::string> &label_set() const { return label_set_; }

  //! Get the top label set (as returned by ProcessAndStripXMLTags).
  const std::map<std::string, int> &top_label_set() const {
    return top_label_set_;
  }

 private:
  void AttachWords(const std::vector<std::string> &, SyntaxTree &);

  std::string sentence_;
  SyntaxNodeCollection node_collection_;
  std::set<std::string> label_set_;
  std::map<std::string, int> top_label_set_;
  std::vector<std::string> words_;
};

}  // namespace Syntax
}  // namespace MosesTraining
