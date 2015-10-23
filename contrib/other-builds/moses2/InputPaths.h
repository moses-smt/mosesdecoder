/*
 * InputPaths.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#ifndef INPUTPATHS_H_
#define INPUTPATHS_H_

#include <vector>
#include "InputPath.h"

class Phrase;

class InputPaths {
public:
	InputPaths(const Phrase &input);
	virtual ~InputPaths();

protected:
	std::vector<InputPath> m_inputPaths;
};

#endif /* INPUTPATHS_H_ */
