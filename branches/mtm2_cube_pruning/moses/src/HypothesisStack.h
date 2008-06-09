
#pragma once

#include <vector>

class Hypothesis;

class HypothesisStack
{
protected:
	typedef std::set< Hypothesis*, HypothesisRecombinationOrderer > _HCType;
	_HCType m_hypos; /**< contains hypotheses */

public:
	typedef _HCType::iterator iterator;
	typedef _HCType::const_iterator const_iterator;
	//! iterators
	const_iterator begin() const { return m_hypos.begin(); }
	const_iterator end() const { return m_hypos.end(); }
	size_t size() const { return m_hypos.size(); }

	virtual bool AddPrune(Hypothesis *hypothesis) = 0;
	virtual const Hypothesis *GetBestHypothesis() const = 0;
	virtual std::vector<const Hypothesis*> GetSortedList() const = 0;
};

