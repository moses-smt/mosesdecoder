#include "LexicalTable.h"

#include "util/tokenize_piece.hh"

#include <cstdlib>
#include <iostream>

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

LexicalTable::LexicalTable(Vocabulary &srcVocab, Vocabulary &tgtVocab)
  : m_srcVocab(srcVocab)
  , m_tgtVocab(tgtVocab)
{
}

void LexicalTable::Load(std::istream &input)
{
  const util::AnyCharacter delimiter(" \t");

  std::string line;
  std::string tmp;
  int i = 0;
  while (getline(input, line)) {
    ++i;
    if (i%100000 == 0) {
      std::cerr << ".";
    }

    util::TokenIter<util::AnyCharacter> it(line, delimiter);

    // Target word
    it->CopyToString(&tmp);
    Vocabulary::IdType tgtId = m_tgtVocab.Insert(tmp);
    ++it;

    // Source word.
    it->CopyToString(&tmp);
    Vocabulary::IdType srcId = m_srcVocab.Insert(tmp);
    ++it;

    // Probability.
    it->CopyToString(&tmp);
    double prob = atof(tmp.c_str());
    m_table[srcId][tgtId] = prob;
  }
  std::cerr << std::endl;
}

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining
