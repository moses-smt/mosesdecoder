#pragma once

#include <istream>
#include <ostream>
#include <string>

#include "syntax-common/tool.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

struct Options;
class SplitPointFileParser;

class PostprocessEgretForests : public Tool
{
public:
  PostprocessEgretForests() : Tool("postprocess-egret-forests") {}

  virtual int Main(int argc, char *argv[]);

private:
  void OneBestTree(std::istream &, std::ostream &, SplitPointFileParser *,
                   const Options &);

  void ProcessForest(std::istream &, std::ostream &, SplitPointFileParser *,
                     const Options &);

  void ProcessOptions(int, char *[], Options &) const;
};

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
