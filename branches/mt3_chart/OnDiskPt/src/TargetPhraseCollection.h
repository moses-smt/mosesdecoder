#pragma once

#include "TargetPhrase.h"
#include "Vocab.h"
	
namespace Moses
{
	class TargetPhraseCollection;
	class PhraseDictionary;
	class LMList;
}

namespace OnDiskPt
{

class TargetPhraseCollection
{
protected:
	typedef std::vector<TargetPhrase*> CollType;
	CollType m_coll;
	Moses::UINT64 m_filePos;
	std::string m_debugStr;

public:
	TargetPhraseCollection();
	TargetPhraseCollection(const TargetPhraseCollection &copy);
	
	~TargetPhraseCollection();
	void AddTargetPhrase(TargetPhrase *targetPhrase);
	void Sort(size_t tableLimit);

	void Save(OnDiskWrapper &onDiskWrapper);

	size_t GetSize() const
	{ return m_coll.size(); }	
	Moses::UINT64 GetFilePos() const;

	Moses::TargetPhraseCollection *ConvertToMoses(const std::vector<Moses::FactorType> &inputFactors
																										, const std::vector<Moses::FactorType> &outputFactors
																										, const Moses::PhraseDictionary &phraseDict
																										, const std::vector<float> &weightT
																										, float weightWP
																										, const Moses::LMList &lmList
																										, const Moses::Phrase &sourcePhrase
																										, const std::string &filePath
																										, Vocab &vocab) const;
	void ReadFromFile(size_t tableLimit, Moses::UINT64 filePos, OnDiskWrapper &onDiskWrapper);

	const std::string GetDebugStr() const;
	void SetDebugStr(const std::string &str);

};

}

