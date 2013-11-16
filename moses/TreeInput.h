#ifndef moses_TreeInput_h
#define moses_TreeInput_h


#include <vector>
#include "Sentence.h"

namespace Moses
{
//! @todo what is this?
class XMLParseOutput
{
public:
  std::string m_label;
  WordsRange m_range;

  XMLParseOutput(const std::string &label, const WordsRange &range)
    : m_label(label)
    , m_range(range) {
  }
};

/** An input to the decoder that represent a parse tree.
 *  Implemented as a sentence with non-terminal labels over certain ranges.
 *  This representation doesn't necessarily have to form a tree, it's up to the user to make sure it does if they really want a tree.
 *  @todo Need to rewrite if you want packed forest, or packed forest over lattice - not sure if can inherit from this
 */
class TreeInput : public Sentence
{
  friend std::ostream& operator<<(std::ostream&, const TreeInput&);

protected:
  std::vector<std::vector<NonTerminalSet> > m_sourceChart;

  void AddChartLabel(size_t startPos, size_t endPos, const std::string &label
                     ,const std::vector<FactorType>& factorOrder);
  void AddChartLabel(size_t startPos, size_t endPos, const Word &label
                     ,const std::vector<FactorType>& factorOrder);
  NonTerminalSet &GetLabelSet(size_t startPos, size_t endPos) {
    return m_sourceChart[startPos][endPos - startPos];
  }

  bool ProcessAndStripXMLTags(std::string &line, std::vector<XMLParseOutput> &sourceLabels, std::vector<XmlOption*> &res);

public:
  TreeInput() {
  }

  InputTypeEnum GetType() const {
    return TreeInputType;
  }

  //! populate this InputType with data from in stream
  virtual int Read(std::istream& in,const std::vector<FactorType>& factorOrder);

  //! Output debugging info to stream out
  virtual void Print(std::ostream&) const;

  //! create trans options specific to this InputType
  virtual TranslationOptionCollection* CreateTranslationOptionCollection() const;

  virtual const NonTerminalSet &GetLabelSet(size_t startPos, size_t endPos) const {
    return m_sourceChart[startPos][endPos - startPos];
  }
};

}

#endif
