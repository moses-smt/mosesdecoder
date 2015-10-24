/*
 * InputPaths.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <iostream>
#include "InputPaths.h"
#include "Phrase.h"
#include "moses/WordsRange.h"

using namespace std;

InputPaths::InputPaths(const Phrase &input)
{
  size_t size = input.GetSize();
  for (size_t phaseSize = 1; phaseSize <= size; ++phaseSize) {
	for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
	  size_t endPos = startPos + phaseSize -1;

	  SubPhrase subPhrase = input.GetSubPhrase(startPos, endPos);
	  Moses::WordsRange range(startPos, endPos);

	  InputPath path(subPhrase, range);
	  m_inputPaths.push_back(path);
	}
  }

}

InputPaths::~InputPaths() {
	// TODO Auto-generated destructor stub
}

