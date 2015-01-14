/*
 * AlignedSentence.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include <set>
#include "ConsistentPhrases.h"
#include "Phrase.h"
#include "moses/TypeDef.h"

class Parameter;

class AlignedSentence
{
public:
  AlignedSentence(int lineNum)
    :m_lineNum(lineNum) {
  }

  AlignedSentence(int lineNum,
                  const std::string &source,
                  const std::string &target,
                  const std::string &alignment);
  virtual ~AlignedSentence();
  virtual void Create(const Parameter &params);

  const Phrase &GetPhrase(Moses::FactorDirection direction) const {
    return (direction == Moses::Input) ? m_source : m_target;
  }

  const ConsistentPhrases &GetConsistentPhrases() const {
    return m_consistentPhrases;
  }

  virtual std::string Debug() const;

  int m_lineNum;
protected:
  Phrase m_source, m_target;
  ConsistentPhrases m_consistentPhrases;

  void CreateConsistentPhrases(const Parameter &params);
  void PopulateWordVec(Phrase &vec, const std::string &line);

  // m_source and m_target MUST be populated before calling this
  void PopulateAlignment(const std::string &line);
  std::vector<int> GetSourceAlignmentCount() const;
};


