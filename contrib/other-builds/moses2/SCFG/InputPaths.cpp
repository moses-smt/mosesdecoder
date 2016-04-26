/*
 * InputPaths.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <iostream>
#include "InputPaths.h"
#include "../Sentence.h"
#include "../System.h"
#include "../legacy/Range.h"
#include "Manager.h"
#include "InputPath.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{
void InputPaths::Init(const Sentence &input, const ManagerBase &mgr)
{
  MemPool &pool = mgr.GetPool();
  size_t numPt = mgr.system.mappings.size();
  size_t size = input.GetSize();
  size_t maxLength = min(size, mgr.system.options.search.max_phrase_length) + 1;

  m_matrix = new (pool.Allocate< Matrix<SCFG::InputPath*> >()) Matrix<SCFG::InputPath*>(pool,
      size, maxLength);
  m_matrix->Init(NULL);

  for (size_t startPos = 0; startPos < size; ++startPos) {
    // create path for 0 length string
    Range range(startPos, startPos - 1);
    SubPhrase<Moses2::Word> subPhrase = input.GetSubPhrase(startPos, 0);

    SCFG::InputPath *path = new (pool.Allocate<SCFG::InputPath>()) SCFG::InputPath(pool,
        subPhrase, range, numPt, NULL);
    //cerr << startPos << " "
    //    << " path=" << *path << endl;
    m_inputPaths.push_back(path);
    m_matrix->SetValue(startPos, 0, path);

    // create normal paths of subphrases through the sentence
    const SCFG::InputPath *prefixPath = path;
    for (size_t phaseSize = 1; phaseSize <= maxLength; ++phaseSize) {
      size_t endPos = startPos + phaseSize - 1;

      if (endPos >= size) {
        break;
      }

      SubPhrase<Moses2::Word> subPhrase = input.GetSubPhrase(startPos, phaseSize);
      Range range(startPos, endPos);

      SCFG::InputPath *path = new (pool.Allocate<SCFG::InputPath>()) SCFG::InputPath(pool,
          subPhrase, range, numPt, prefixPath);
      //cerr << startPos << " " << (phaseSize - 1)
      //    << " path=" << *path << endl;
      m_inputPaths.push_back(path);

      prefixPath = path;

      m_matrix->SetValue(startPos, phaseSize, path);
    }
  }

}

}
}

