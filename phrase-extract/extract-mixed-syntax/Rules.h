/*
 * Rules.h
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#pragma once

#include <set>
#include <iostream>
#include "ConsistentPhrases.h"
#include "Rule.h"

extern bool g_debug;

class AlignedSentence;
class Parameter;

struct CompareRules {
  bool operator()(const Rule *a, const Rule *b) {
    int compare;

    compare = a->GetPhrase(Moses::Input).Compare(b->GetPhrase(Moses::Input));
    if (compare) return compare < 0;

    compare = a->GetPhrase(Moses::Output).Compare(b->GetPhrase(Moses::Output));
    if (compare) return compare < 0;

    if (a->GetAlignments() != b->GetAlignments()) {
      return a->GetAlignments() < b->GetAlignments();
    }

    if (a->GetLHS().GetString() != b->GetLHS().GetString()) {
      return a->GetLHS().GetString() < b->GetLHS().GetString();
    }

    if (a->GetProperties() != b->GetProperties()) {
      return a->GetProperties() < b->GetProperties();
    }

    return false;
  }
};

class Rules
{
public:
  Rules(const AlignedSentence &alignedSentence);
  virtual ~Rules();
  void Extend(const Parameter &params);
  void Consolidate(const Parameter &params);

  std::string Debug() const;
  void Output(std::ostream &out, bool forward, const Parameter &params) const;

protected:
  const AlignedSentence &m_alignedSentence;
  std::set<Rule*> m_keepRules;
  std::set<Rule*, CompareRules> m_mergeRules;

  void Extend(const Rule &rule, const Parameter &params);
  void Extend(const Rule &rule, const ConsistentPhrases::Coll &cps, const Parameter &params);
  void Extend(const Rule &rule, const ConsistentPhrase &cp, const Parameter &params);

  // create original rules
  void CreateRules(const ConsistentPhrase &cp,
                   const Parameter &params);
  void CreateRule(const NonTerm &nonTerm,
                  const Parameter &params);

  void MergeRules(const Parameter &params);
  void CalcFractionalCount();

};

