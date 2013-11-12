
#include <stdlib.h>
#include "InputPath.h"
#include "WordsRange.h"
#include "FF/TranslationModel/PhraseTable.h"

InputPath::InputPath(const InputPath *prevPath, const Phrase *phrase, size_t endPos)
  :m_lookupColl(PhraseTable::GetColl().size())
  ,m_prevPath(prevPath)
  ,m_phrase(phrase)
{
  size_t startPos = prevPath ? prevPath->GetRange().startPos : endPos;
  m_range = new WordsRange(startPos, endPos);
}

InputPath::~InputPath()
{
}

