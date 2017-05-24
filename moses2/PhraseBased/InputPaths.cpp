/*
 * InputPaths.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <iostream>
#include "../InputPathsBase.h"
#include "../System.h"
#include "../legacy/Range.h"
#include "Manager.h"
#include "Sentence.h"

using namespace std;

namespace Moses2
{

void InputPaths::Init(const InputType &input, const ManagerBase &mgr)
{
  const Sentence &sentence = static_cast<const Sentence&>(input);

  MemPool &pool = mgr.GetPool();
  size_t numPt = mgr.system.mappings.size();
  size_t size = sentence.GetSize();
  size_t maxLength = min(size, mgr.system.options.search.max_phrase_length);

  m_matrix = new (pool.Allocate<Matrix<InputPath*> >()) Matrix<InputPath*>(pool,
      size, maxLength);
  m_matrix->Init(NULL);

  // create blank path for initial hypo
  Range range(NOT_FOUND, NOT_FOUND);
  SubPhrase<Moses2::Word> subPhrase = sentence.GetSubPhrase(NOT_FOUND, NOT_FOUND);
  m_blank = new (pool.Allocate<InputPath>()) InputPath(pool, subPhrase, range,
      numPt, NULL);

  // create normal paths of subphrases through the sentence
  for (size_t startPos = 0; startPos < size; ++startPos) {
    const InputPath *prefixPath = NULL;

    for (size_t phaseSize = 1; phaseSize <= maxLength; ++phaseSize) {
      size_t endPos = startPos + phaseSize - 1;

      if (endPos >= size) {
        break;
      }

      SubPhrase<Moses2::Word> subPhrase = sentence.GetSubPhrase(startPos, phaseSize);
      Range range(startPos, endPos);

      InputPath *path = new (pool.Allocate<InputPath>()) InputPath(pool,
          subPhrase, range, numPt, prefixPath);
      m_inputPaths.push_back(path);

      prefixPath = path;

      m_matrix->SetValue(startPos, phaseSize - 1, path);
    }
  }

}

}

