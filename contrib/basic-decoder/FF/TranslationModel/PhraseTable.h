/*
 * PhraseTable.h
 *
 *  Created on: 5 Oct 2013
 *      Author: hieu
 */

#pragma once

#include "FF/StatelessFeatureFunction.h"

class InputPath;

class PhraseTable :public StatelessFeatureFunction
{
public:
  static const std::vector<PhraseTable*>& GetColl() {
    return s_staticColl;
  }

  PhraseTable(const std::string line);
  virtual ~PhraseTable();

  virtual void Lookup(const std::vector<InputPath*> &inputPathQueue) = 0;
protected:
  static std::vector<PhraseTable*> s_staticColl;
  static size_t s_ptId;

  size_t m_ptId;

};

