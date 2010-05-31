#pragma once
/*
 *  OnDiskWrapper.h
 *  CreateOnDisk
 *
 *  Created by Hieu Hoang on 31/12/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include <fstream>
#include "Vocab.h"
#include "PhraseNode.h"
#include "../../moses/src/Word.h"

namespace OnDiskPt
{
const float DEFAULT_COUNT = 66666;
	
class OnDiskWrapper
{
protected:
	Vocab m_vocab;
	std::string m_filePath;
	int m_numSourceFactors, m_numTargetFactors, m_numScores;
	std::fstream m_fileMisc, m_fileVocab, m_fileSource, m_fileTarget, m_fileTargetInd, m_fileTargetColl;

	size_t m_defaultNodeSize;
	PhraseNode *m_rootSourceNode;

	std::map<std::string, UINT64> m_miscInfo;
	
	void SaveMisc();
	bool OpenForLoad(const std::string &filePath);
	bool LoadMisc();

public:
	OnDiskWrapper();
	~OnDiskWrapper();

	bool BeginLoad(const std::string &filePath);

	bool BeginSave(const std::string &filePath
								 , int numSourceFactors, int	numTargetFactors, int numScores);
	void EndSave();
	
	Vocab &GetVocab()
	{ return m_vocab; }
	
	size_t GetSourceWordSize() const;
	size_t GetTargetWordSize() const;
	
	std::fstream &GetFileSource()
	{ return m_fileSource; }
	std::fstream &GetFileTargetInd()
	{ return m_fileTargetInd; }
	std::fstream &GetFileTargetColl()
	{ return m_fileTargetColl; }
	std::fstream &GetFileVocab()
	{ return m_fileVocab; }
	
	size_t GetNumSourceFactors() const
	{ return m_numSourceFactors; }
	size_t GetNumTargetFactors() const
	{ return m_numTargetFactors; }
	
	size_t GetNumScores() const
	{ return m_numScores; }
	size_t GetNumCounts() const
	{ return 1; }
	
	PhraseNode &GetRootSourceNode();
	
	UINT64 GetMisc(const std::string &key) const;

	Word *ConvertFromMoses(Moses::FactorDirection direction
											, const std::vector<Moses::FactorType> &factorsVec
											, const Moses::Word &origWord) const;

};

}
