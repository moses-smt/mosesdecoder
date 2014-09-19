#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "CorpusLine.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <exception>

using namespace std;
using namespace Moses;

CorpusLine::CorpusLine(const std::string &fileName)
{
	InputFileStream in(fileName);
	if (! in.good())
		throw runtime_error("error: cannot open " + fileName);
	string line;
	int currentId = 1;
	while (getline(in, line)) {
		//We assume that sentence ID begins at 1
		AddLine(currentId,line);
		currentId++;
  }
}

void CorpusLine::AddLine(int lineNbr, std::string &line)
{
	 pair<CorpusEntry::iterator, bool> ret = m_entry.insert(make_pair(lineNbr, line));
	 //std::cerr << "Corpus Line : inserting " << lineNbr << " " << line << std::endl;
}

string CorpusLine::GetLine(int lineNbr)
{
	CorpusEntry::const_iterator it = m_entry.find(lineNbr);
	  if (it == m_entry.end())
	    throw logic_error("error: trying to read unknown corpus line" + lineNbr);
	  return it->second;
}
