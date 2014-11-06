#ifndef PSDLINE_H_
#define PSDLINE_H_

#include "moses/Util.h";


class PSDLine
{

friend std::ostream& operator<<(std::ostream& out, const PSDLine& possibleLine)
{
	  out << "PSD line with : " << std::endl
	  	  << "Source Side : " << possibleLine.GetSrcPhrase() << std::endl
	      << "Source Span : " << Moses::SPrint(possibleLine.GetSrcStart()) << "-" << Moses::SPrint(possibleLine.GetSrcEnd()) << std::endl
	      << "Target Side : " << possibleLine.GetTgtPhrase() << std::endl
	      << "Sentence Id : " << possibleLine.GetSentID() << std::endl;
	  return out;
}


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
  const std::string &GetSrcPhrase() const { return m_srcPhrase; }
  const std::string &GetTgtPhrase() const { return m_tgtPhrase; }
  size_t GetSentID() const { return m_sentID; }
  size_t GetSrcStart() const { return m_srcStart; }
  size_t GetSrcEnd() const { return m_srcEnd; }

private:
  PSDLine();
  size_t m_sentID, m_srcStart, m_srcEnd;
  std::string m_srcPhrase, m_tgtPhrase;
};

#endif /* PSDLINE_H_ */
