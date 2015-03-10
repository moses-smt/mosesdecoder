#pragma once

#include <istream>
#include <ostream>
#include <string>

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

struct Options;
class SplitPointFileParser;

class PostprocessEgretForests
{
public:
  PostprocessEgretForests() : m_name("postprocess-egret-forests") {}

  void Error(const std::string &) const;

  const std::string &GetName() const {
    return m_name;
  }

  int Main(int argc, char *argv[]);

private:
  void OneBestTree(std::istream &, std::ostream &, SplitPointFileParser *,
                   const Options &);

  void OpenInputFileOrDie(const std::string &, std::ifstream &);

  void ProcessForest(std::istream &, std::ostream &, SplitPointFileParser *,
                     const Options &);

  void ProcessOptions(int, char *[], Options &) const;

  std::string m_name;
};

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
