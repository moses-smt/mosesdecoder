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
#ifndef PCFG_SYNTAX_TREE_H_
#define PCFG_SYNTAX_TREE_H_

#include <cassert>
#include <vector>

namespace Moses
{
namespace PCFG
{

// Base class for SyntaxTree, AgreementTree, and friends.
template<typename T, typename DerivedType>
class SyntaxTreeBase
{
public:
  // Constructors
  SyntaxTreeBase(const T &label)
    : label_(label)
    , children_()
    , parent_(0) {}

  SyntaxTreeBase(const T &label, const std::vector<DerivedType *> &children)
    : label_(label)
    , children_(children)
    , parent_(0) {}

  // Destructor
  virtual ~SyntaxTreeBase();

  const T &label() const {
    return label_;
  }
  const DerivedType *parent() const {
    return parent_;
  }
  DerivedType *parent() {
    return parent_;
  }
  const std::vector<DerivedType *> &children() const {
    return children_;
  }
  std::vector<DerivedType *> &children() {
    return children_;
  }

  void set_label(const T &label) {
    label_ = label;
  }
  void set_parent(DerivedType *parent) {
    parent_ = parent;
  }
  void set_children(const std::vector<DerivedType *> &c) {
    children_ = c;
  }

  bool IsLeaf() const {
    return children_.empty();
  }

  bool IsPreterminal() const {
    return children_.size() == 1 && children_[0]->IsLeaf();
  }

  void AddChild(DerivedType *child) {
    children_.push_back(child);
  }

private:
  T label_;
  std::vector<DerivedType *> children_;
  DerivedType *parent_;
};

template<typename T>
class SyntaxTree : public SyntaxTreeBase<T, SyntaxTree<T> >
{
public:
  typedef SyntaxTreeBase<T, SyntaxTree<T> > BaseType;
  SyntaxTree(const T &label) : BaseType(label) {}
  SyntaxTree(const T &label, const std::vector<SyntaxTree *> &children)
    : BaseType(label, children) {}
};

template<typename T, typename DerivedType>
SyntaxTreeBase<T, DerivedType>::~SyntaxTreeBase()
{
  for (std::size_t i = 0; i < children_.size(); ++i) {
    delete children_[i];
  }
}

}  // namespace PCFG
}  // namespace Moses

#endif
