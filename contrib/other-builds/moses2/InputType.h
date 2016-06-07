/*
 * InputType.h
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */

#pragma once

#include "ReorderingConstraint.h"

namespace Moses2
{

class InputType
{
public:
  InputType(long translationId) :
      m_translationId(translationId)
  {
  }

  virtual ~InputType();

  virtual void Init(size_t size, int max_distortion);

  long GetTranslationId() const
  {
    return m_translationId;
  }

  ReorderingConstraint &GetReorderingConstraint()
  { return m_reorderingConstraint; }

  const ReorderingConstraint &GetReorderingConstraint() const
  { return m_reorderingConstraint; }

protected:
  long m_translationId; 	//< contiguous Id
  ReorderingConstraint m_reorderingConstraint; /**< limits on reordering specified either by "-mp" switch or xml tags */

};

} /* namespace Moses2 */

