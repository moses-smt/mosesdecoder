/*
 * Rule.cpp
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */

#include <limits>
#include <cassert>
#include "Rule.h"
#include "Parameter.h"
#include "LatticeArc.h"
#include "ConsistentPhrases.h"
#include "AlignedSentence.h"

using namespace std;

Rule::Rule(const LatticeArc &arc)
:m_isValid(true)
,m_canExtend(true)
{
	m_arcs.push_back(&arc);
}

Rule::Rule(const Rule &prevRule, const LatticeArc &arc)
:m_arcs(prevRule.m_arcs)
,m_isValid(true)
,m_canExtend(true)
{
	m_arcs.push_back(&arc);
}

Rule::~Rule() {
	// TODO Auto-generated destructor stub
}

bool Rule::IsValid(const Parameter &params) const
{
  if (!m_isValid) {
	  return false;
  }

  return true;
}

bool Rule::CanExtend(const Parameter &params) const
{
  return true;
}

void Rule::Fillout(const ConsistentPhrases &consistentPhrases,
				const AlignedSentence &alignedSentence)
{
  // if last word is a non-term, check to see if it overlaps with any other non-terms
  if (m_arcs.back()->IsNonTerm()) {
	  const ConsistentRange *sourceRange = static_cast<const ConsistentRange *>(m_arcs.back());
	  const ConsistentRange &lastTargetRange = sourceRange->GetOtherRange();

	  for (size_t i = 0; i < m_arcs.size() - 1; ++i) {
		  const LatticeArc *arc = m_arcs[i];

		  if (arc->IsNonTerm()) {
			  const ConsistentRange *sourceRange = static_cast<const ConsistentRange *>(arc);
			  const ConsistentRange &targetRange = sourceRange->GetOtherRange();

			  if (lastTargetRange.Overlap(targetRange)) {
				  m_isValid = false;
				  m_canExtend = false;
				  return;
			  }
		  }
	  }
  }

  // find out if it's a consistent phrase
  int sourceStart = m_arcs.front()->GetStart();
  int sourceEnd = m_arcs.back()->GetEnd();

  int targetStart = numeric_limits<int>::max();
  int targetEnd = -1;

  for (size_t i = 0; i < m_arcs.size(); ++i) {
	  const LatticeArc &arc = *m_arcs[i];
	  if (arc.GetStart() < targetStart) {
		  targetStart = arc.GetStart();
	  }
	  if (arc.GetEnd() > targetEnd) {
		  targetEnd = arc.GetEnd();
	  }
  }

  m_consistentPhrase = consistentPhrases.Find(sourceStart, sourceEnd, targetStart, targetEnd);
  if (m_consistentPhrase == NULL) {
	  m_isValid = false;
	  return;
  }

  // everything looks ok, create target phrase
  // get a list of all target non-term
  vector<const ConsistentRange*> targetNonTerms;
  for (size_t i = 0; i < m_arcs.size(); ++i) {
	  const LatticeArc *arc = m_arcs[i];

	  if (arc->IsNonTerm()) {
		  const ConsistentRange *sourceRange = static_cast<const ConsistentRange *>(arc);
		  const ConsistentRange &targetRange = sourceRange->GetOtherRange();
		  targetNonTerms.push_back(&targetRange);
	  }
  }

  // targetNonTerms will be deleted element-by-element as it is used
  CreateTargetPhrase(alignedSentence.GetPhrase(Moses::Output),
		  targetStart,
		  targetEnd,
		  targetNonTerms);
  //assert(targetNonTerms.size() == 0);
}

void Rule::CreateTargetPhrase(const Phrase &targetPhrase,
		int targetStart,
		int targetEnd,
		vector<const ConsistentRange*> &targetNonTerms)
{
	for (int pos = targetStart; pos <= targetEnd; ++pos) {
		const ConsistentRange *range = Overlap(pos, targetNonTerms);
		if (range) {
			// part of non-term.
			m_targetArcs.push_back(range);

			pos = range->GetEnd();
		}
		else {
			// just use the word
			const Word *word = targetPhrase[pos];
			m_targetArcs.push_back(word);
		}
	}
}

const ConsistentRange *Rule::Overlap(int pos, vector<const ConsistentRange*> &targetNonTerms)
{
	vector<const ConsistentRange*>::iterator iter;
	for (iter = targetNonTerms.begin(); iter != targetNonTerms.end(); ++iter) {
		const ConsistentRange *range = *iter;
		if (range->Overlap(pos)) {
			// is part of a non-term. Delete the range
			targetNonTerms.erase(iter);
			return range;
		}
	}

	return NULL;
}

Rule *Rule::Extend(const LatticeArc &arc) const
{
	Rule *ret = new Rule(*this, arc);

	return ret;
}

void Rule::Output(std::ostream &out, const std::vector<const LatticeArc*> &arcs) const
{
  for (size_t i = 0; i < arcs.size(); ++i) {
	  const LatticeArc &arc = *arcs[i];
	  arc.Output(out);
	  out << " ";
  }
}

void Rule::Output(std::ostream &out) const
{
	Output(out, m_arcs);
	out << " ||| ";
	Output(out, m_arcs);
}
