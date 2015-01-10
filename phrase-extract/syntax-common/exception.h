#pragma once

#include <string>

namespace MosesTraining {
namespace Syntax {

class Exception {
 public:
  Exception(const char *msg) : msg_(msg) {}
  Exception(const std::string &msg) : msg_(msg) {}

  const std::string &msg() const { return msg_; }

 private:
  std::string msg_;
};

}  // namespace Syntax
}  // namespace MosesTraining
