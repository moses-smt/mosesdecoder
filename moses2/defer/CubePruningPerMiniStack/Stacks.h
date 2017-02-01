/*
 * Stacks.h
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "../CubePruningMiniStack/Stack.h"
#include "../../Recycler.h"

namespace Moses2
{
class Manager;

namespace NSCubePruningPerMiniStack
{

class Stacks
{
  friend std::ostream& operator<<(std::ostream &, const Stacks &);
public:
  Stacks(const Manager &mgr);
  virtual ~Stacks();

  void Init(size_t numStacks);

  size_t GetSize() const {
    return m_stacks.size();
  }

  const NSCubePruningMiniStack::Stack &Back() const {
    return *m_stacks.back();
  }

  NSCubePruningMiniStack::Stack &operator[](size_t ind) {
    return *m_stacks[ind];
  }

  void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);
  NSCubePruningMiniStack::MiniStack &GetMiniStack(const Bitmap &newBitmap, const Range &pathRange);

protected:
  const Manager &m_mgr;
  std::vector<NSCubePruningMiniStack::Stack*> m_stacks;
};


}

}


