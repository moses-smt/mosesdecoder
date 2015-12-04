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
#include "MemPool.h"

class PhraseImpl;
class System;

class InputPaths {
	typedef std::vector<InputPath> Coll;
public:
	InputPaths() {}
	void Init(const PhraseImpl &input, const System &system);
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

  void DeleteUnusedPaths();

protected:
	Coll m_inputPaths;
};

#endif /* INPUTPATHS_H_ */
