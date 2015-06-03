#pragma once

#include <ostream>
#include <string>

#include "SyntaxTree.h"

namespace MosesTraining {
namespace Syntax {

class XmlTreeWriter {
 public:
  XmlTreeWriter(std::ostream &out, bool escape=true)
      : out_(out)
      , escape_(escape) {}

  void Write(const SyntaxTree &) const;

 private:
  std::string Escape(const std::string &) const;

  std::ostream &out_;
  bool escape_;
};

}  // namespace Syntax
}  // namespace MosesTraining
