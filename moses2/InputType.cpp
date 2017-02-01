/*
 * InputType.cpp
 *
 *  Created on: 14 Dec 2015
 *      Author: hieu
 */

#include "InputType.h"
#include "System.h"

namespace Moses2
{
//////////////////////////////////////////////////////////////////////////////
InputType::XMLOption::XMLOption(MemPool &pool, const std::string &nodeName, size_t vStartPos)
  :startPos(vStartPos)
  ,prob(0)
  ,m_entity(NULL)
{
  m_nodeName = pool.Allocate<char>(nodeName.size() + 1);
  strcpy(m_nodeName, nodeName.c_str());
}

void InputType::XMLOption::SetTranslation(MemPool &pool, const std::string &val)
{
  m_translation = pool.Allocate<char>(val.size() + 1);
  strcpy(m_translation, val.c_str());
}

void InputType::XMLOption::SetEntity(MemPool &pool, const std::string &val)
{
  m_entity = pool.Allocate<char>(val.size() + 1);
  strcpy(m_entity, val.c_str());
}

std::string InputType::XMLOption::Debug(const System &system) const
{
  std::stringstream out;
  out << "[" << startPos << "," << phraseSize << "]="
      << m_nodeName << ","
      << m_translation << ","
      << prob;
  if (m_entity) {
    out << "," << m_entity;
  }
  return out.str();
}

//////////////////////////////////////////////////////////////////////////////

InputType::InputType(MemPool &pool)
  :m_reorderingConstraint(pool)
  ,m_xmlOptions(pool)
  ,m_xmlCoverageMap(pool)
{
}

InputType::~InputType()
{
  // TODO Auto-generated destructor stub
}

void InputType::Init(const System &system, size_t size, int max_distortion)
{
  m_reorderingConstraint.InitializeWalls(size, max_distortion);

  if (system.options.input.xml_policy != XmlPassThrough) {
    m_xmlCoverageMap.assign(size, false);
  }
}

void InputType::AddXMLOption(const System &system, const XMLOption *xmlOption)
{
  m_xmlOptions.push_back(xmlOption);

  if (system.options.input.xml_policy != XmlPassThrough) {
    for(size_t j = xmlOption->startPos; j < xmlOption->startPos + xmlOption->phraseSize; ++j) {
      m_xmlCoverageMap[j]=true;
    }
  }
}

bool InputType::XmlOverlap(size_t startPos, size_t endPos) const
{
  for (size_t pos = startPos; pos <=  endPos ; pos++) {
    if (pos < m_xmlCoverageMap.size() && m_xmlCoverageMap[pos]) {
      return true;
    }
  }
  return false;
}

} /* namespace Moses2 */
