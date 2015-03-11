#include "PostprocessEgretForests.h"

#include "syntax-common/exception.h"

int main(int argc, char *argv[])
{
  MosesTraining::Syntax::PostprocessEgretForests::PostprocessEgretForests tool;
  try {
    return tool.Main(argc, argv);
  } catch (const MosesTraining::Syntax::Exception &e) {
    tool.Error(e.msg());
  }
}
