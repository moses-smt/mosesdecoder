#pragma once

#include <string>

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

struct Options {
public:
  Options()
    : goodTuring(false)
    , inverse(false)
    , kneserNey(false)
    , logProb(false)
    , minCountHierarchical(0)
    , negLogProb(false)
    , noLex(false)
    , noWordAlignment(false)
    , treeScore(false) {}

  // Positional options
  std::string extractFile;
  std::string lexFile;
  std::string tableFile;

  // All other options
  bool goodTuring;
  bool inverse;
  bool kneserNey;
  bool logProb;
  int minCountHierarchical;
  bool negLogProb;
  bool noLex;
  bool noWordAlignment;
  bool treeScore;
};

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining
