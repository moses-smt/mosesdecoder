#pragma once

#include <string>

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

struct Options {
public:
  Options() : escape(false) {}

  bool escape;
  std::string splitPointsFile;
};

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace Moses
