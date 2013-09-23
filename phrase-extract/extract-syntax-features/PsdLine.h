#ifndef PSDLINE_H_
#define PSDLINE_H_

#include "Util.h";

class PSDLine
{
public:
  PSDLine(const std::string &line)
  {
	std::vector<std::string> columns = Moses::Tokenize(line, "\t");
    m_sentID   = Moses::Scan<size_t>(columns[0]);
    m_srcStart = Moses::Scan<size_t>(columns[1]);
    m_srcEnd   = Moses::Scan<size_t>(columns[2]);
    m_srcPhrase = columns[5];
    m_tgtPhrase = columns[6];
  }
  const std::string &GetSrcPhrase() { return m_srcPhrase; }
  const std::string &GetTgtPhrase() { return m_tgtPhrase; }
  size_t GetSentID()    { return m_sentID; }
  size_t GetSrcStart()  { return m_srcStart; }
  size_t GetSrcEnd()    { return m_srcEnd; }

private:
  PSDLine();
  size_t m_sentID, m_srcStart, m_srcEnd;
  std::string m_srcPhrase, m_tgtPhrase;
};


#endif /* PSDLINE_H_ */
