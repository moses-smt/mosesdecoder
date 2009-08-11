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
#include <db.h>
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
	
class SourceKey
{
public:
	SourceKey(long sourceId, int vocabId)
	:m_sourceId(sourceId)
	,m_vocabId(vocabId)
	{	}
	
	long m_sourceId;
	int m_vocabId;
	
};

class DbWrapper
{
	Db m_dbMisc, m_dbVocab, m_dbSource, m_dbTarget, m_dbTargetInd, m_dbTargetColl;
	Vocab m_vocab;
	long m_nextSourceNodeId, m_nextTargetNodeId;
	int m_numSourceFactors, m_numTargetFactors, m_numScores;
	SourcePhraseNode m_initNode;
	bool m_openSave;
	
	long SaveSourceWord(long currSourceNodeId, const Word &word);
	
	void SetMisc(const std::string &key, int value);
	void OpenFiles(const std::string &filePath);

	void SaveVocab();
	void SaveMisc();

public:
	DbWrapper();
	~DbWrapper();
	void BeginSave(const std::string &filePath
								, int numSourceFactors, int	numTargetFactors, int numScores);
	void EndSave();

	void Load(const std::string &filePath);

	long SaveSource(const Phrase &phrase, const TargetPhrase &target);
	void SaveTarget(TargetPhrase &phrase);
	void SaveTargetPhraseCollection(long sourceNodeId, const TargetPhraseCollection &tpColl);

	Word *ConvertFromMoses(Moses::FactorDirection direction
												, const std::vector<Moses::FactorType> &factorsVec
												, const Moses::Word &origWord) const;

	Word *CreateSouceWord() const;
	Word *CreateTargetWord() const;
	
	const SourcePhraseNode &GetInitNode() const
	{ return m_initNode; }
	
	size_t GetSourceWordSize() const
	{
		return m_numSourceFactors * sizeof(VocabId) + sizeof(char);
	}
	size_t GetTargetWordSize() const
	{
		return m_numTargetFactors * sizeof(VocabId) + sizeof(char);
	}

	Vocab &GetVocab()
	{ return m_vocab; }

	int GetMisc(const std::string &key);

	const SourcePhraseNode *GetChild(const SourcePhraseNode &parentNode, const Word &word);
	const TargetPhraseCollection *GetTargetPhraseCollection(const SourcePhraseNode &node);

	const Moses::TargetPhraseCollection *ConvertToMoses(const TargetPhraseCollection &tpColl
																											, const std::vector<Moses::FactorType> &inputFactors
																											, const std::vector<Moses::FactorType> &outputFactors
																											, const Moses::PhraseDictionary &phraseDict
																											, const std::vector<float> &weightT
																											, float weightWP
																											, const Moses::LMList &lmList
																											, const Moses::Phrase &sourcePhrase) const;

};

}; // namespace

