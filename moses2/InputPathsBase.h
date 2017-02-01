/*
 * InputPaths.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "MemPool.h"

namespace Moses2
{

class InputType;
class System;
class ManagerBase;
class InputPathBase;

class InputPathsBase
{
  typedef std::vector<InputPathBase*> Coll;
public:
  InputPathsBase() {
  }
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

  virtual void Init(const InputType &input, const ManagerBase &mgr) = 0;

protected:
  Coll m_inputPaths;
};

}

