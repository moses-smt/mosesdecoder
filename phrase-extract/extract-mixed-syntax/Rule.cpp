/*
 * Rule.cpp
 *
 *  Created on: 20 Feb 2014
 *      Author: hieu
 */

#include <sstream>
#include <algorithm>
#include "Rule.h"
#include "AlignedSentence.h"
#include "ConsistentPhrase.h"
#include "NonTerm.h"
#include "Parameter.h"

using namespace std;

Rule::Rule(const NonTerm &lhsNonTerm, const AlignedSentence &alignedSentence)
  :m_lhs(lhsNonTerm)
  ,m_alignedSentence(alignedSentence)
  ,m_isValid(true)
  ,m_canRecurse(true)
{
  CreateSource();
}

Rule::Rule(const Rule &copy, const NonTerm &nonTerm)
  :m_lhs(copy.m_lhs)
  ,m_alignedSentence(copy.m_alignedSentence)
  ,m_isValid(true)
  ,m_canRecurse(true)
  ,m_nonterms(copy.m_nonterms)
{
  m_nonterms.push_back(&nonTerm);
  CreateSource();

}

Rule::~Rule()
{
  // TODO Auto-generated destructor stub
}

const ConsistentPhrase &Rule::GetConsistentPhrase() const
{
  return m_lhs.GetConsistentPhrase();
}

void Rule::CreateSource()
{
  const NonTerm *cp = NULL;
  size_t nonTermInd = 0;
  if (nonTermInd < m_nonterms.size()) {
    cp = m_nonterms[nonTermInd];
  }

  for (int sourcePos = m_lhs.GetConsistentPhrase().corners[0];
       sourcePos <= m_lhs.GetConsistentPhrase().corners[1];
       ++sourcePos) {

    const RuleSymbol *ruleSymbol;
    if (cp && cp->GetConsistentPhrase().corners[0] <= sourcePos && sourcePos <= cp->GetConsistentPhrase().corners[1]) {
      // replace words with non-term
      ruleSymbol = cp;
      sourcePos = cp->GetConsistentPhrase().corners[1];
      if (m_nonterms.size()) {
        cp = m_nonterms[nonTermInd];
      }

      // move to next non-term
      ++nonTermInd;
      cp = (nonTermInd < m_nonterms.size()) ? m_nonterms[nonTermInd] : NULL;
    } else {
      // terminal
      ruleSymbol = m_alignedSentence.GetPhrase(Moses::Input)[sourcePos];
    }

    m_source.Add(ruleSymbol);
  }
}

int Rule::GetNextSourcePosForNonTerm() const
{
  if (m_nonterms.empty()) {
    // no non-terms so far. Can start next non-term on left corner
    return m_lhs.GetConsistentPhrase().corners[0];
  } else {
    // next non-term can start just left of previous
    const ConsistentPhrase &cp = m_nonterms.back()->GetConsistentPhrase();
    int nextPos = cp.corners[1] + 1;
    return nextPos;
  }
}

std::string Rule::Debug() const
{
  stringstream out;

  // source
  for (size_t i =  0; i < m_source.GetSize(); ++i) {
    const RuleSymbol &symbol = *m_source[i];
    out << symbol.Debug() << " ";
  }

  // target
  out << "||| ";
  for (size_t i =  0; i < m_target.GetSize(); ++i) {
    const RuleSymbol &symbol = *m_target[i];
    out << symbol.Debug() << " ";
  }

  out << "||| ";
  Alignments::const_iterator iterAlign;
  for (iterAlign =  m_alignments.begin(); iterAlign != m_alignments.end(); ++iterAlign) {
    const std::pair<int,int> &alignPair = *iterAlign;
    out << alignPair.first << "-" << alignPair.second << " ";
  }

  // overall range
  out << "||| LHS=" << m_lhs.Debug();

  return out.str();
}

void Rule::Output(std::ostream &out, bool forward, const Parameter &params) const
{
  if (forward) {
    // source
    m_source.Output(out);
    m_lhs.Output(out, Moses::Input);

    out << " ||| ";

    // target
    m_target.Output(out);
    m_lhs.Output(out, Moses::Output);
  } else {
    // target
    m_target.Output(out);
    m_lhs.Output(out, Moses::Output);

    out << " ||| ";

    // source
    m_source.Output(out);
    m_lhs.Output(out, Moses::Input);
  }

  out << " ||| ";

  // alignment
  Alignments::const_iterator iterAlign;
  for (iterAlign =  m_alignments.begin(); iterAlign != m_alignments.end(); ++iterAlign) {
    const std::pair<int,int> &alignPair = *iterAlign;

    if (forward) {
      out << alignPair.first << "-" << alignPair.second << " ";
    } else {
      out << alignPair.second << "-" << alignPair.first << " ";
    }
  }

  out << "||| ";

  // count
  out << m_count;

  out << " ||| ";

  // properties

  // span length
  if (forward && params.spanLength && m_nonterms.size()) {
    out << "{{SpanLength ";

    for (size_t i = 0; i < m_nonterms.size(); ++i) {
      const NonTerm &nonTerm = *m_nonterms[i];
      const ConsistentPhrase &cp = nonTerm.GetConsistentPhrase();
      out << i << "," << cp.GetWidth(Moses::Input) << "," << cp.GetWidth(Moses::Output) << " ";
    }
    out << "}} ";
  }

  // non-term context (source)
  if (forward && params.nonTermContext && m_nonterms.size()) {
    out << "{{NonTermContext ";

    int factor = params.nonTermContextFactor;

    for (size_t i = 0; i < m_nonterms.size(); ++i) {
      const NonTerm &nonTerm = *m_nonterms[i];
      const ConsistentPhrase &cp = nonTerm.GetConsistentPhrase();
      NonTermContext(1, factor, i, cp, out);
    }
    out << "}} ";
  }

  // non-term context (target)
  if (forward && params.nonTermContextTarget && m_nonterms.size()) {
    out << "{{NonTermContextTarget ";

    int factor = params.nonTermContextFactor;

    for (size_t i = 0; i < m_nonterms.size(); ++i) {
      const NonTerm &nonTerm = *m_nonterms[i];
      const ConsistentPhrase &cp = nonTerm.GetConsistentPhrase();
      NonTermContext(2, factor, i, cp, out);
    }
    out << "}} ";
  }

}

void Rule::NonTermContextFactor(int factor, const Word &word, std::ostream &out) const
{
  out << word.GetString(factor) << " ";
}

void Rule::NonTermContext(int sourceTarget, int factor, size_t ntInd, const ConsistentPhrase &cp, std::ostream &out) const
{
  int startPos, endPos;
  const Phrase *phrase;

  if (sourceTarget == 1) {
    startPos = cp.corners[0];
    endPos = cp.corners[1];
    phrase = &m_alignedSentence.GetPhrase(Moses::Input);
  } else if (sourceTarget == 2) {
    startPos = cp.corners[2];
    endPos = cp.corners[3];
    phrase = &m_alignedSentence.GetPhrase(Moses::Output);
  } else {
    abort();
  }

  out << ntInd << " ";

  // left outside
  if (startPos == 0) {
    out << "<s> ";
  } else {
    NonTermContextFactor(factor, *phrase->at(startPos - 1), out);
  }

  // left inside
  NonTermContextFactor(factor, *phrase->at(startPos), out);

  // right inside
  NonTermContextFactor(factor, *phrase->at(endPos), out);

  // right outside
  if (endPos == phrase->size() - 1) {
    out << "</s> ";
  } else {
    NonTermContextFactor(factor, *phrase->at(endPos + 1), out);
  }


}

void Rule::Prevalidate(const Parameter &params)
{
  const ConsistentPhrase &cp = m_lhs.GetConsistentPhrase();

  // check number of source symbols in rule
  if (m_source.GetSize() > params.maxSymbolsSource) {
    m_isValid = false;
  }

  // check that last non-term added isn't too small
  if (m_nonterms.size()) {
    const NonTerm &lastNonTerm = *m_nonterms.back();
    const ConsistentPhrase &cp = lastNonTerm.GetConsistentPhrase();

    int sourceWidth = cp.GetWidth(Moses::Input);
    if (lastNonTerm.IsHiero(params)) {
      if (sourceWidth < params.minHoleSource) {
        m_isValid = false;
        m_canRecurse = false;
        return;
      }
    } else if (sourceWidth < params.minHoleSourceSyntax) {
      m_isValid = false;
      m_canRecurse = false;
      return;
    }

  }

  // check number of non-terms
  int numNonTerms = 0;
  int numHieroNonTerms = 0;
  for (size_t i = 0; i < m_source.GetSize(); ++i) {
    const RuleSymbol *arc = m_source[i];
    if (arc->IsNonTerm()) {
      ++numNonTerms;
      const NonTerm &nonTerm = *static_cast<const NonTerm*>(arc);
      bool isHiero = nonTerm.IsHiero(params);
      if (isHiero) {
        ++numHieroNonTerms;
      }
    }
  }

  if (numNonTerms >= params.maxNonTerm) {
    m_canRecurse = false;
    if (numNonTerms > params.maxNonTerm) {
      m_isValid = false;
      return;
    }
  }

  if (numHieroNonTerms >= params.maxHieroNonTerm) {
    m_canRecurse = false;
    if (numHieroNonTerms > params.maxHieroNonTerm) {
      m_isValid = false;
      return;
    }
  }

  // check if 2 consecutive non-terms in source
  if (!params.nonTermConsecSource && m_nonterms.size() >= 2) {
    const NonTerm &lastNonTerm = *m_nonterms.back();
    const NonTerm &secondLastNonTerm = *m_nonterms[m_nonterms.size() - 2];
    if (secondLastNonTerm.GetConsistentPhrase().corners[1] + 1 ==
        lastNonTerm.GetConsistentPhrase().corners[0]) {
      if (params.mixedSyntaxType == 0) {
        // ordinary hiero or syntax model
        m_isValid = false;
        m_canRecurse = false;
        return;
      } else {
        // Hieu's mixed syntax
        switch (params.nonTermConsecSourceMixedSyntax) {
        case 0:
          m_isValid = false;
          m_canRecurse = false;
          return;
        case 1:
          if (lastNonTerm.IsHiero(Moses::Input, params)
              && secondLastNonTerm.IsHiero(Moses::Input, params)) {
            m_isValid = false;
            m_canRecurse = false;
            return;
          }
          break;
        case 2:
          if (lastNonTerm.IsHiero(Moses::Input, params)
              || secondLastNonTerm.IsHiero(Moses::Input, params)) {
            m_isValid = false;
            m_canRecurse = false;
            return;
          }
          break;
        case 3:
          break;
        } // switch
      }
    }
  }

  //check to see if it overlaps with any other non-terms
  if (m_nonterms.size() >= 2) {
    const NonTerm &lastNonTerm = *m_nonterms.back();

    for (size_t i = 0; i < m_nonterms.size() - 1; ++i) {
      const NonTerm &otherNonTerm = *m_nonterms[i];
      bool overlap = lastNonTerm.GetConsistentPhrase().TargetOverlap(otherNonTerm.GetConsistentPhrase());

      if (overlap) {
        m_isValid = false;
        m_canRecurse = false;
        return;
      }
    }
  }

  // check that at least 1 word is aligned
  if (params.requireAlignedWord) {
    bool ok = false;
    for (size_t i = 0; i < m_source.GetSize(); ++i) {
      const RuleSymbol &symbol = *m_source[i];
      if (!symbol.IsNonTerm()) {
        const Word &word = static_cast<const Word&>(symbol);
        if (word.GetAlignment().size()) {
          ok = true;
          break;
        }
      }
    }

    if (!ok) {
      m_isValid = false;
      m_canRecurse = false;
      return;
    }
  }

  if (params.maxSpanFreeNonTermSource) {
    const NonTerm *front = dynamic_cast<const NonTerm*>(m_source[0]);
    if (front) {
      int width = front->GetWidth(Moses::Input);
      if (width > params.maxSpanFreeNonTermSource) {
        m_isValid = false;
        m_canRecurse = false;
        return;
      }
    }

    const NonTerm *back = dynamic_cast<const NonTerm*>(m_source.Back());
    if (back) {
      int width = back->GetWidth(Moses::Input);
      if (width > params.maxSpanFreeNonTermSource) {
        m_isValid = false;
        m_canRecurse = false;
        return;
      }
    }
  }

  if (!params.nieceTerminal) {
    // collect terminal in a rule
    std::set<const Word*> terms;
    for (size_t i = 0; i < m_source.GetSize(); ++i) {
      const Word *word = dynamic_cast<const Word*>(m_source[i]);
      if (word) {
        terms.insert(word);
      }
    }

    // look in non-terms
    for (size_t i = 0; i < m_source.GetSize(); ++i) {
      const NonTerm *nonTerm = dynamic_cast<const NonTerm*>(m_source[i]);
      if (nonTerm) {
        const ConsistentPhrase &cp = nonTerm->GetConsistentPhrase();
        bool containTerm = ContainTerm(cp, terms);

        if (containTerm) {
          //cerr << "ruleSource=" << *ruleSource << " ";
          //cerr << "ntRange=" << ntRange << endl;

          // non-term contains 1 of the terms in the rule.
          m_isValid = false;
          m_canRecurse = false;
          return;
        }
      }
    }
  }

  if (params.maxScope != UNDEFINED || params.minScope > 0) {
    int scope = GetScope(params);
    if (scope > params.maxScope) {
      // scope of subsequent rules will be the same or increase
      // therefore can NOT recurse
      m_isValid = false;
      m_canRecurse = false;
      return;
    }

    if (scope < params.minScope) {
      // scope of subsequent rules may increase
      // therefore can recurse
      m_isValid = false;
    }
  }

  // min/max span per scope
  if (params.scopeSpan.size()) {
    int scope = GetScope(params);
    if (scope >= params.scopeSpan.size()) {
      // no constraint on it. It's ok
    } else {
      const std::pair<int,int> &constraint = params.scopeSpan[scope];
      int sourceWidth = m_lhs.GetWidth(Moses::Input);
      if (sourceWidth < constraint.first || sourceWidth > constraint.second) {
        m_isValid = false;
        m_canRecurse = false;
        return;
      }
    }
  }
}

int Rule::GetScope(const Parameter &params) const
{
  size_t scope = 0;
  bool previousIsAmbiguous = false;

  if (m_source[0]->IsNonTerm()) {
    scope++;
    previousIsAmbiguous = true;
  }

  for (size_t i = 1; i < m_source.GetSize(); ++i) {
    const RuleSymbol *symbol = m_source[i];
    bool isAmbiguous = symbol->IsNonTerm();
    if (isAmbiguous) {
      // mixed syntax
      const NonTerm *nt = static_cast<const NonTerm*>(symbol);
      isAmbiguous = nt->IsHiero(Moses::Input, params);
    }

    if (isAmbiguous && previousIsAmbiguous) {
      scope++;
    }
    previousIsAmbiguous = isAmbiguous;
  }

  if (previousIsAmbiguous) {
    scope++;
  }

  return scope;

  /*
  int scope = 0;
  if (m_source.GetSize() > 1) {
    const RuleSymbol &front = *m_source.Front();
    if (front.IsNonTerm()) {
  	  ++scope;
    }

    const RuleSymbol &back = *m_source.Back();
    if (back.IsNonTerm()) {
  	  ++scope;
    }
  }
  return scope;
  */
}

template<typename T>
bool Contains(const T *sought, const set<const T*> &coll)
{
  std::set<const Word*>::const_iterator iter;
  for (iter = coll.begin(); iter != coll.end(); ++iter) {
    const Word *found = *iter;
    if (sought->CompareString(*found) == 0) {
      return true;
    }
  }
  return false;
}

bool Rule::ContainTerm(const ConsistentPhrase &cp, const std::set<const Word*> &terms) const
{
  const Phrase &sourceSentence = m_alignedSentence.GetPhrase(Moses::Input);

  for (int pos = cp.corners[0]; pos <= cp.corners[1]; ++pos) {
    const Word *soughtWord = sourceSentence[pos];

    // find same word in set
    if (Contains(soughtWord, terms)) {
      return true;
    }
  }
  return false;
}

bool CompareTargetNonTerms(const NonTerm *a, const NonTerm *b)
{
  // compare just start target pos
  return a->GetConsistentPhrase().corners[2] < b->GetConsistentPhrase().corners[2];
}

void Rule::CreateTarget(const Parameter &params)
{
  if (!m_isValid) {
    return;
  }

  vector<const NonTerm*> targetNonTerm(m_nonterms);
  std::sort(targetNonTerm.begin(), targetNonTerm.end(), CompareTargetNonTerms);

  const NonTerm *cp = NULL;
  size_t nonTermInd = 0;
  if (nonTermInd < targetNonTerm.size()) {
    cp = targetNonTerm[nonTermInd];
  }

  for (int targetPos = m_lhs.GetConsistentPhrase().corners[2];
       targetPos <= m_lhs.GetConsistentPhrase().corners[3];
       ++targetPos) {

    const RuleSymbol *ruleSymbol;
    if (cp && cp->GetConsistentPhrase().corners[2] <= targetPos && targetPos <= cp->GetConsistentPhrase().corners[3]) {
      // replace words with non-term
      ruleSymbol = cp;
      targetPos = cp->GetConsistentPhrase().corners[3];
      if (targetNonTerm.size()) {
        cp = targetNonTerm[nonTermInd];
      }

      // move to next non-term
      ++nonTermInd;
      cp = (nonTermInd < targetNonTerm.size()) ? targetNonTerm[nonTermInd] : NULL;
    } else {
      // terminal
      ruleSymbol = m_alignedSentence.GetPhrase(Moses::Output)[targetPos];
    }

    m_target.Add(ruleSymbol);
  }

  CreateAlignments();
}


void Rule::CreateAlignments()
{
  int sourceStart = GetConsistentPhrase().corners[0];
  int targetStart = GetConsistentPhrase().corners[2];

  for (size_t sourcePos = 0; sourcePos < m_source.GetSize(); ++sourcePos) {
    const RuleSymbol *symbol = m_source[sourcePos];
    if (!symbol->IsNonTerm()) {
      // terminals
      const Word &sourceWord = static_cast<const Word&>(*symbol);
      const std::set<const Word *> &targetWords = sourceWord.GetAlignment();
      CreateAlignments(sourcePos, targetWords);
    } else {
      // non-terms. same object in both source & target
      CreateAlignments(sourcePos, symbol);
    }
  }
}

void Rule::CreateAlignments(int sourcePos, const std::set<const Word *> &targetWords)
{
  std::set<const Word *>::const_iterator iterTarget;
  for (iterTarget = targetWords.begin(); iterTarget != targetWords.end(); ++iterTarget) {
    const Word *targetWord = *iterTarget;
    CreateAlignments(sourcePos, targetWord);
  }
}

void Rule::CreateAlignments(int sourcePos, const RuleSymbol *targetSought)
{
  // should be in target phrase
  for (size_t targetPos = 0; targetPos < m_target.GetSize(); ++targetPos) {
    const RuleSymbol *foundSymbol = m_target[targetPos];
    if (targetSought == foundSymbol) {
      pair<int, int> alignPoint(sourcePos, targetPos);
      m_alignments.insert(alignPoint);
      return;
    }
  }

  throw "not found";
}

