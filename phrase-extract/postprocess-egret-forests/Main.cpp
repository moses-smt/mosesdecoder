#include "PostprocessEgretForests.h"

#include "syntax-common/exception.h"

int main(int argc, char *argv[])
{
  MosesTraining::Syntax::PostprocessEgretForests::PostprocessEgretForests tool;
  return tool.Main(argc, argv);
}
