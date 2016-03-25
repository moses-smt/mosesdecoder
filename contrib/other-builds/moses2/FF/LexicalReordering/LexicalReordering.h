/*
 * LexicalReordering.h
 *
 *  Created on: 15 Dec 2015
 *      Author: hieu
 */

#pragma once
#include <vector>
#include <boost/unordered_map.hpp>
#include "../StatefulFeatureFunction.h"
#include "../../TypeDef.h"
#include "../../Phrase.h"
#include "../../legacy/Range.h"

namespace Moses2 {

class LexicalReorderingTableCompact;
class LRModel;

class LexicalReordering : public StatefulFeatureFunction
{
public:
  LexicalReordering(size_t startInd, const std::string &line);
  virtual ~LexicalReordering();

  virtual void Load(System &system);

  virtual void SetParameter(const std::string& key, const std::string& value);

  virtual size_t HasPhraseTableInd() const
  { return true; }

  virtual FFState* BlankState(MemPool &pool) const;
  virtual void EmptyHypothesisState(FFState &state,
		  const ManagerBase &mgr,
		  const InputType &input,
		  const Hypothesis &hypo) const;

  virtual void
  EvaluateInIsolation(MemPool &pool,
		  const System &system,
		  const Phrase &source,
		  const TargetPhrase &targetPhrase,
		  Scores &scores,
		  SCORE *estimatedScore) const;

  virtual void
  EvaluateAfterTablePruning(MemPool &pool,
		  const TargetPhrases &tps,
		  const Phrase &sourcePhrase) const;

  virtual void EvaluateWhenApplied(const ManagerBase &mgr,
	const Hypothesis &hypo,
	const FFState &prevState,
	Scores &scores,
	FFState &state) const;

protected:
  std::string m_path;
  FactorList m_FactorsF;
  FactorList m_FactorsE;
  FactorList m_FactorsC;

  LRModel *m_configuration;

  virtual void
  EvaluateAfterTablePruning(MemPool &pool,
		  const TargetPhrase &targetPhrase,
		  const Phrase &sourcePhrase) const;

  // PROPERTY IN PT
  int m_propertyInd;

  // COMPACT MODEL
  LexicalReorderingTableCompact *m_compactModel;
  Phrase *m_blank;

  // MEMORY MODEL
  typedef std::pair<const Phrase*, const Phrase*> Key;
  typedef std::vector<SCORE> Values;

  struct KeyComparer
  {
	  size_t operator()(const Key &obj) const {
		  size_t  seed = obj.first->hash();
		  boost::hash_combine(seed, obj.second->hash());
		  return seed;
	  }

	  bool operator()(const Key& a, const Key& b) const {
		if ((*a.first) != (*b.first)) {
			return false;
		}
		if ((*a.second) != (*b.second)) {
			return false;
		}
	    return true;
	  }

  };

  typedef boost::unordered_map<Key, Values, KeyComparer, KeyComparer> Coll;
  Coll *m_coll;

  const Values *GetValues(const Phrase &source, const Phrase &target) const;
};

} /* namespace Moses2 */

