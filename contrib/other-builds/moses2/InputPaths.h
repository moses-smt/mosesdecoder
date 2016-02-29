/*
 * InputPaths.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "PhraseBased/InputPath.h"
#include "MemPool.h"
#include "legacy/Matrix.h"

namespace Moses2
{

class Sentence;
class System;
class Manager;

class InputPathsBase
{
	typedef std::vector<InputPath*> Coll;
public:
	InputPathsBase() {}
	virtual ~InputPathsBase();

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

  virtual void Init(const Sentence &input, const Manager &mgr) = 0;

  const Matrix<InputPath*> &GetMatrix() const
  { return *m_matrix; }

protected:
	Coll m_inputPaths;
	Matrix<InputPath*> *m_matrix;
};

class InputPaths : public InputPathsBase
{
public:
	  void Init(const Sentence &input, const Manager &mgr);

	  const InputPath &GetBlank() const
	  { return *m_blank; }

protected:
		InputPath *m_blank;

};

}


