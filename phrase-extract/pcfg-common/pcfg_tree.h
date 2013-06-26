/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#pragma once
#ifndef PCFG_PCFG_TREE_H_
#define PCFG_PCFG_TREE_H_

#include "syntax_tree.h"
#include "xml_tree_writer.h"

#include <string>

namespace Moses
{
namespace PCFG
{

template<typename DerivedType>
class PcfgTreeBase : public SyntaxTreeBase<std::string, DerivedType>
{
public:
  typedef std::string LabelType;
  typedef SyntaxTreeBase<LabelType, DerivedType> BaseType;

  PcfgTreeBase(const LabelType &label) : BaseType(label), score_(0.0) {}

  double score() const {
    return score_;
  }
  void set_score(double s) {
    score_ = s;
  }

private:
  double score_;
};

class PcfgTree : public PcfgTreeBase<PcfgTree>
{
public:
  typedef PcfgTreeBase<PcfgTree> BaseType;
  PcfgTree(const BaseType::LabelType &label) : BaseType(label) {}
};

// Specialise XmlOutputHandler for PcfgTree.
template<>
class XmlOutputHandler<PcfgTree>
{
public:
  typedef std::map<std::string, std::string> AttributeMap;

  void GetLabel(const PcfgTree &tree, std::string &label) const {
    label = tree.label();
  }

  void GetAttributes(const PcfgTree &tree, AttributeMap &attribute_map) const {
    attribute_map.clear();
    double score = tree.score();
    if (score != 0.0) {
      std::ostringstream out;
      out << tree.score();
      attribute_map["pcfg"] = out.str();
    }
  }
};

}  // namespace PCFG
}  // namespace Moses

#endif
