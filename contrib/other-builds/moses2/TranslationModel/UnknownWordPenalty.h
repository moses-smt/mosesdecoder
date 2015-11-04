/*
 * UnknownWordPenalty.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#ifndef UNKNOWNWORDPENALTY_H_
#define UNKNOWNWORDPENALTY_H_

#include "PhraseTable.h"

class UnknownWordPenalty : public PhraseTable
{
public:
	UnknownWordPenalty(size_t startInd, const std::string &line);
	virtual ~UnknownWordPenalty();

	virtual TargetPhrases::shared_const_ptr Lookup(const Manager &mgr, InputPath &inputPath) const;

	  virtual void
	  EvaluateInIsolation(const System &system,
			  const Phrase &source, const TargetPhrase &targetPhrase,
			  Scores &scores,
			  Scores *estimatedScores) const;

};

#endif /* UNKNOWNWORDPENALTY_H_ */
