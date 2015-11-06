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
#include "moses/Range.h"

using namespace std;

InputPaths::~InputPaths() {
	// TODO Auto-generated destructor stub
}

void InputPaths::Init(const PhraseImpl &input, const System &system)
{
  size_t numPt = system.mappings.size();
  size_t size = input.GetSize();
  for (size_t phaseSize = 1; phaseSize <= min(size, system.maxPhraseLength) ; ++phaseSize) {
  //for (size_t phaseSize = 1; phaseSize <= size; ++phaseSize) {
	for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
	  size_t endPos = startPos + phaseSize -1;

	  SubPhrase subPhrase = input.GetSubPhrase(startPos, endPos);
	  Moses::Range range(startPos, endPos);

	  InputPath path(subPhrase, range, numPt);
	  m_inputPaths.push_back(path);
	}
  }

}

void InputPaths::DeleteUnusedPaths()
{
	size_t ind = 0;
	while (ind < m_inputPaths.size()) {
		const InputPath &path = m_inputPaths[ind];
		if (path.IsUsed()) {
			m_inputPaths.erase(m_inputPaths.begin() + ind);
		}
		else {
			++ind;
		}
	}
}

