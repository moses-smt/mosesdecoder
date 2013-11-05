#ifndef moses_InputTreeRep_h
#define moses_InputTreeRep_h

#include <vector>
#include <string>
#include "util/check.hh"

namespace Moses
{

class WordsRangeForTrain
{
    private :
    size_t m_startPos;
    size_t m_endPos;

    public :
    WordsRangeForTrain(size_t startPos, size_t endPos);

    size_t GetStartPos() const
    {
        return m_startPos;
    }

    size_t GetEndPos() const
    {
        return m_endPos;
    }
};

//! @todo what is this?
class XMLParseOutputForTrain
{
public:
  std::string m_label;
  WordsRangeForTrain m_range;

  XMLParseOutputForTrain(const std::string &label, const WordsRangeForTrain &range)
    : m_label(label)
    , m_range(range)
  {}
};

/**
Reimplementation of InputTree for extracting syntax features during training
**/

class SyntaxLabel
{
	public:
	std::string m_label;
	bool m_nonTerm;
	SyntaxLabel(const std::string &label, bool isNonTerm);

	const std::string GetString() const
	{return m_label;}

	const bool IsNonTerm() const
	{return m_nonTerm;}

	void clear()
	{
	    m_label.erase();
	}
};

class InputTreeRep
{

typedef std::vector<SyntaxLabel> SyntLabels;

public:
  std::vector<std::vector<SyntLabels> > m_sourceChart;
  std::string m_noTag;

  void AddChartLabel(size_t startPos, size_t endPos, SyntaxLabel label);

  SyntLabels GetLabels(size_t startPos, size_t endPos) const {
    //std::cerr << "GETTING LABEL FOR : " << startPos << " : " << endPos - startPos << std::endl;
    CHECK( !(m_sourceChart.size() < startPos) );
    CHECK( !(m_sourceChart[startPos].size() < (endPos - startPos) ) );
    return m_sourceChart[startPos][endPos - startPos];
  }

  SyntLabels GetRelLabels(size_t startPos, size_t endPos) const {
    //std::cerr << "GETTING RELATIVE LABEL FOR : " << startPos << " : " << endPos << std::endl;
    return m_sourceChart[startPos][endPos];
  }

  std::string GetNoTag() const
  {
        return m_noTag;
  }

  SyntaxLabel GetParent(size_t startPos, size_t endPos, bool &IsBegin) const;

  size_t ProcessXMLTags(std::string &line, std::vector<XMLParseOutputForTrain> &sourceLabelss);

  //in case sentence size is not known when constructed
  void PopulateChart(size_t sourceSize);

public:
  InputTreeRep(size_t sourceSize);

  //! populate this InputType with data from in stream
  int Read(std::string &in);

  //! Output debugging info to stream out
  void Print(size_t size) const;
};

}

#endif
