/*
 * PhraseTableMemory.h
 *
 *  Created on: 5 Oct 2013
 *      Author: hieu
 */

#pragma once

#include <string>
#include "PhraseTable.h"
#include "Memory/Node.h"

class PhraseTableMemory: public PhraseTable
{
public:
  PhraseTableMemory(const std::string &line);
  virtual ~PhraseTableMemory();

  void Load();
  void SetParameter(const std::string& key, const std::string& value);

  virtual void Evaluate(const Phrase &source
                        , const TargetPhrase &targetPhrase
                        , Scores &scores
                        , Scores &estimatedFutureScore) const {
  }

  void Lookup(const std::vector<InputPath*> &inputPathQueue);
protected:
  std::string m_path;
  size_t m_tableLimit;

  Node m_root;

};

