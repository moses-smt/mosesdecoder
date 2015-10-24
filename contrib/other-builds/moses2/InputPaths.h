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
	typedef std::vector<InputPath> Coll;
public:
	InputPaths() {}
	void Init(const Phrase &input);
	virtual ~InputPaths();

  //! iterators
  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;

  const_iterator begin() const {
	return m_inputPaths.begin();
  }
  const_iterator end() const {
	return m_inputPaths.end();
  }

  iterator begin() {
	return m_inputPaths.begin();
  }
  iterator end() {
	return m_inputPaths.end();
  }

protected:
	Coll m_inputPaths;
};

#endif /* INPUTPATHS_H_ */
