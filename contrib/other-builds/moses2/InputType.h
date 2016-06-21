/*
 * InputType.h
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */

#pragma once

#include "ReorderingConstraint.h"
#include "TypeDef.h"

namespace Moses2
{

class InputType
{
public:
  //////////////////////////////////////////////////////////////////////////////
  class XMLOption
  {
  public:
	std::string nodeName;
	size_t startPos, phraseSize;

	std::string translation;
	SCORE prob;

	void Debug(std::ostream &out, const System &system) const;

  };

  //////////////////////////////////////////////////////////////////////////////

  InputType(long translationId, MemPool &pool);
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

  std::vector<XMLOption*> &GetXMLOptions()
  { return m_xmlOptions; }

  const std::vector<XMLOption*> &GetXMLOptions() const
  { return m_xmlOptions; }

protected:
  long m_translationId; 	//< contiguous Id
  ReorderingConstraint m_reorderingConstraint; /**< limits on reordering specified either by "-mp" switch or xml tags */
  std::vector<XMLOption*> m_xmlOptions;
};

} /* namespace Moses2 */

