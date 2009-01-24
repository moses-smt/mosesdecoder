
#include <fstream>
#include "Vocab.h"

using namespace std;

namespace MosesOnDiskPt
{

Vocab::Vocab()
:m_nextId(1)
{}

VocabId Vocab::AddFactor(const std::string &factorString)
{
	// find string id
	CollType::const_iterator iter = m_vocabColl.find(factorString);
	if (iter == m_vocabColl.end())
	{ // add new vocab entry
		m_vocabColl[factorString] = m_nextId;
		return m_nextId++;
	}
	else
	{ // return existing entry
		return iter->second;
	}
}
void Vocab::Save(const std::string &filePath)
{
	ofstream fileStream(filePath.c_str(), ios::out);
	
	CollType::iterator iter;
	for (iter = m_vocabColl.begin(); iter != m_vocabColl.end(); ++iter)
	{
		fileStream << iter->first << "\t" << iter->second << endl;
	}

	fileStream.close();
}

}

