/*
 * Stacks.h
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "Stack.h"
#include "../../Recycler.h"

namespace Moses2
{
class Manager;
class ArcLists;

namespace NSNormal
{

class Stacks
{
public:
  Stacks(const Manager &mgr);
  virtual ~Stacks();

  void Init(const Manager &mgr, size_t numStacks);

  size_t GetSize() const {
    return m_stacks.size();
  }

  const Stack &Back() const {
    return *m_stacks.back();
  }

  Stack &operator[](size_t ind) {
    return *m_stacks[ind];
  }

  void Delete(size_t ind) {
    delete m_stacks[ind];
    m_stacks[ind] = NULL;
  }

  void Add(Hypothesis *hypo, Recycler<HypothesisBase*> &hypoRecycle,
           ArcLists &arcLists);

  std::string Debug(const System &system) const;

protected:
  const Manager &m_mgr;
  std::vector<Stack*> m_stacks;
};

}
}
