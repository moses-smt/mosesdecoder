#pragma once

/*
 *  Db.h
 *  CreateBerkeleyPt
 *
 *  Created by Hieu Hoang on 29/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
#include <db_cxx.h>
#include "Vocab.h"
#include "SourcePhraseNode.h"
#include "../../moses/src/TypeDef.h"
#include "../../moses/src/Word.h"
#include "../../moses/src/TargetPhraseCollection.h"

namespace MosesBerkeleyPt
{

class Phrase;
class TargetPhrase;
class TargetPhraseCollection;
class Word;
		
class DbWrapper
{
	Db m_dbMisc, m_dbVocab, m_dbSource, m_dbTarget, m_dbTargetInd, m_dbTargetColl;
	Vocab m_vocab;
	Moses::UINT32 m_nextSourceNodeId;
	Moses::UINT32 m_nextTargetNodeId;
	int m_numSourceFactors, m_numTargetFactors, m_numScores;
	SourcePhraseNode m_initNode;
	bool m_openSave;
	
	
	void SetMisc(const std::string &key, Moses::UINT32 value);
	bool OpenForSave(const std::string &filePath);
	bool OpenForLoad(const std::string &filePath);
	bool OpenDb(Db &db, const std::string &filePath, DBTYPE type, u_int32_t flags, int mode);

	void SaveVocab();
	void SaveMisc();

public:
	DbWrapper();
	~DbWrapper();
	bool BeginSave(const std::string &filePath
								, int numSourceFactors, int	numTargetFactors, int numScores);
	void EndSave();

	bool Load(const std::string &filePath);

	void SaveTarget(TargetPhrase &phrase);
	void SaveTargetPhraseCollection(Moses::UINT32 sourceNodeId, const TargetPhraseCollection &tpColl);

	Word *ConvertFromMoses(Moses::FactorDirection direction
												, const std::vector<Moses::FactorType> &factorsVec
												, const Moses::Word &origWord) const;

	Word *CreateSouceWord() const;
	Word *CreateTargetWord() const;
	Moses::UINT32 &GetNextSourceNodeId();
	
	const SourcePhraseNode &GetInitNode() const
	{ return m_initNode; }
	
	size_t GetSourceWordSize() const;
	size_t GetTargetWordSize() const;

	Vocab &GetVocab()
	{ return m_vocab; }

	Moses::UINT32 GetMisc(const std::string &key);
	
	Db &GetSourceDb()
	{ return m_dbSource; }

	const TargetPhraseCollection *GetTargetPhraseCollection(const SourcePhraseNode &node);
	const SourcePhraseNode *GetChild(const SourcePhraseNode &parentNode, const Word &word);
	const TargetPhraseCollection *GetTargetPhraseCollection(const SourcePhraseNode &node, float &sourceCount, float &entropy);

	Moses::TargetPhraseCollection *ConvertToMoses(const TargetPhraseCollection &tpColl
																											, const std::vector<Moses::FactorType> &inputFactors
																											, const std::vector<Moses::FactorType> &outputFactors
																											, const Moses::PhraseDictionary &phraseDict
																											, const std::vector<float> &weightT
																											, float weightWP
																											, const Moses::LMList &lmList
																											, const Moses::Phrase &sourcePhrase) const;

};

}; // namespace

