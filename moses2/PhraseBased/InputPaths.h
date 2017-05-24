/*
 * InputPaths.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "InputPath.h"
#include "../MemPool.h"
#include "../InputPathsBase.h"
#include "../legacy/Matrix.h"

namespace Moses2
{

class System;

class InputPaths: public InputPathsBase
{
public:
  void Init(const InputType &input, const ManagerBase &mgr);

  const InputPath &GetBlank() const {
    return *m_blank;
  }

  Matrix<InputPath*> &GetMatrix() {
    return *m_matrix;
  }

  const Matrix<InputPath*> &GetMatrix() const {
    return *m_matrix;
  }

protected:
  InputPath *m_blank;
  Matrix<InputPath*> *m_matrix;

};

}

