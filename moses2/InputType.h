/*
 * InputType.h
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */

#pragma once

#include "PhraseBased/ReorderingConstraint.h"
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
    size_t startPos, phraseSize;

    SCORE prob;

    XMLOption(MemPool &pool, const std::string &nodeName, size_t vStartPos);

    const char *GetNodeName() const {
      return m_nodeName;
    }

    const char *GetTranslation() const {
      return m_translation;
    }

    const char *GetEntity() const {
      return m_entity;
    }

    void SetTranslation(MemPool &pool, const std::string &val);
    void SetEntity(MemPool &pool, const std::string &val);

    std::string Debug(const System &system) const;
  public:
    char *m_nodeName;
    char *m_translation;
    char *m_entity;

  };

  //////////////////////////////////////////////////////////////////////////////

  InputType(MemPool &pool);
  virtual ~InputType();

  virtual void Init(const System &system, size_t size, int max_distortion);

  ReorderingConstraint &GetReorderingConstraint() {
    return m_reorderingConstraint;
  }

  const ReorderingConstraint &GetReorderingConstraint() const {
    return m_reorderingConstraint;
  }

  const Vector<const XMLOption*> &GetXMLOptions() const {
    return m_xmlOptions;
  }

  void AddXMLOption(const System &system, const XMLOption *xmlOption);

  //! Returns true if there were any XML tags parsed that at least partially covered the range passed
  bool XmlOverlap(size_t startPos, size_t endPos) const;

protected:
  ReorderingConstraint m_reorderingConstraint; /**< limits on reordering specified either by "-mp" switch or xml tags */
  Vector<const XMLOption*> m_xmlOptions;
  Vector<bool> m_xmlCoverageMap;

};

} /* namespace Moses2 */

