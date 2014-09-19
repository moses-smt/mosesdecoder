#ifndef CORPUSLINE_H_
#define CORPUSLINE_H_

#include "moses/Util.h";

typedef std::map<int,std::string> CorpusEntry;

class CorpusLine
{

public:
	CorpusLine(const std::string &fileName);
	void AddLine(int lineNbr, std::string &line);
	std::string GetLine(int lineNbr);

private:
  CorpusEntry m_entry;
};


#endif /* CORPUSLINE_H_ */
