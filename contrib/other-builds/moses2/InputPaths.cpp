/*
 * InputPaths.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <iostream>
#include "InputPaths.h"
#include "Phrase.h"
#include "System.h"
#include "legacy/Range.h"

using namespace std;

namespace Moses2
{

InputPaths::~InputPaths() {
	// TODO Auto-generated destructor stub
}

void InputPaths::Init(const PhraseImpl &input, const System &system)
{
  size_t numPt = system.mappings.size();
  size_t size = input.GetSize();
  size_t maxLength = min(size, system.maxPhraseLength);

  for (size_t startPos = 0; startPos < size; ++startPos) {
	const InputPath *prefixPath = NULL;

    for (size_t phaseSize = 1; phaseSize <= maxLength; ++phaseSize) {
	  size_t endPos = startPos + phaseSize - 1;

	  if (endPos >= size) {
		  break;
	  }

	  SubPhrase subPhrase = input.GetSubPhrase(startPos, endPos);
	  Range range(startPos, endPos);

	  InputPath path(subPhrase, range, numPt, prefixPath);
	  m_inputPaths.push_back(path);

	  prefixPath = &m_inputPaths.back();
	}
  }

}

void InputPaths::DeleteUnusedPaths()
{
	size_t ind = 0;
	while (ind < m_inputPaths.size()) {
		const InputPath &path = m_inputPaths[ind];
		if (!path.IsUsed()) {
			m_inputPaths.erase(m_inputPaths.begin() + ind);
		}
		else {
			++ind;
		}
	}
}

}

