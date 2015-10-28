/*
 * PhraseTableMemory.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#ifndef PHRASETABLEMEMORY_H_
#define PHRASETABLEMEMORY_H_
#include "PhraseTable.h"

class PhraseTableMemory : public PhraseTable
{
public:
	PhraseTableMemory(size_t startInd, const std::string &line);
	virtual ~PhraseTableMemory();
};

#endif /* PHRASETABLEMEMORY_H_ */
