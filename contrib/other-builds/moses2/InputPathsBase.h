/*
 * InputPaths.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "MemPool.h"
#include "legacy/Matrix.h"

namespace Moses2
{

class Sentence;
class System;
class ManagerBase;
class InputPathBase;

class InputPathsBase
{
	typedef std::vector<InputPathBase*> Coll;
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

  virtual void Init(const Sentence &input, const ManagerBase &mgr) = 0;

  const Matrix<InputPathBase*> &GetMatrix() const
  { return *m_matrix; }

  /** Get a future cost score for a span */
  inline const InputPathBase &GetInputPath(size_t row, size_t col) const {
    return *m_matrix->GetValue(row, col);
  }

  inline InputPathBase &GetInputPath(size_t row, size_t col) {
    return *m_matrix->GetValue(row, col);
  }

protected:
	Coll m_inputPaths;
	Matrix<InputPathBase*> *m_matrix;
};

}


