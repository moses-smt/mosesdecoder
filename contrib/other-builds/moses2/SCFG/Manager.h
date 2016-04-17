/*
 * Manager.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <queue>
#include <cstddef>
#include <string>
#include <deque>
#include "../ManagerBase.h"
#include "Stacks.h"
#include "InputPaths.h"

namespace Moses2
{

namespace SCFG
{

class Manager: public Moses2::ManagerBase
{
public:
  Manager(System &sys, const TranslationTask &task, const std::string &inputStr,
      long translationId);

  virtual ~Manager();
  void Decode();
  std::string OutputBest() const {}
  std::string OutputNBest() {}

protected:
  Stacks m_stacks;
  InputPaths m_inputPaths;

  void InitActiveChart(size_t pos);
  void Lookup(size_t startPos, size_t size);
  void Decode(size_t startPos, size_t size);
};

}
}

