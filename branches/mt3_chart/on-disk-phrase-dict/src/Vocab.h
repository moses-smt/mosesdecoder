
#pragma once

#include <map>
#include <string>
#include "TypeDef.h"

namespace MosesOnDiskPt
{

class Vocab
{
protected:
	typedef std::map<std::string, VocabId> CollType;
	CollType m_vocabColl;
	VocabId m_nextId; // starts @ 1

public:
	Vocab();
	VocabId AddFactor(const std::string &factorString);

	void Save(const std::string &filePath);

};

}

