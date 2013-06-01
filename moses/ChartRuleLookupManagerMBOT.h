//Fabienne Braune
//Rule Manager working with l-MBOT ChartCells. This new class could be avoided by having a pointer to ChartCellCollectionBase as field in
//class ChartRuleLookupManager.

#pragma once
#ifndef moses_ChartRuleLookupManagerMBOT_h
#define moses_ChartRuleLookupManagerMBOT_h

#include "ChartRuleLookupManager.h"
#include "InputType.h"

namespace Moses
{

class ChartParserCallback;
class WordsRange;

/** Defines an interface for looking up rules in a rule table.  Concrete
 *  implementation classes should correspond to specific PhraseDictionary
 *  subclasses (memory or on-disk).  Since a ChartRuleLookupManager object
 *  maintains sentence-specific state, exactly one should be created for
 *  each sentence that is to be decoded.
 */
class ChartRuleLookupManagerMBOT : public ChartRuleLookupManager
{
public:
  ChartRuleLookupManagerMBOT(const InputType &sentence,
                         const ChartCellCollectionBase &cellColl)
    :ChartRuleLookupManager(sentence, cellColl)
    , m_sentence(sentence)
    , m_cellCollection(cellColl) {}

  virtual ~ChartRuleLookupManagerMBOT() {}

  //Fabienne Braune : GetSentence is already defined in ChartRuleLookupManager but cannot be accessed
  //from outside because ambiguous
  const InputType &GetSentenceFromMBOT() const {
    return m_sentence;
  }

  const ChartCellLabelSetMBOT GetMBOTTargetLabelSet(size_t begin, size_t end) const {
    return m_cellCollection.GetBase(WordsRange(begin, end)).GetTargetLabelSetForMBOT();
  }

  const ChartCellLabelMBOT * GetMBOTSourceAt(size_t at) const {
    return m_cellCollection.GetSourceWordLabelMBOT(at);
  }

  size_t GetMBOTSizeAt(size_t at){

      return m_cellCollection.GetSourceWordLabelMBOT(at)->GetLabelMBOT().GetSize();}

  const Word * GetMBOTSourceWordAt(size_t at1, size_t at2) const {
	  //std::cerr << "GETTING SOURCE WORD AT : " << *(m_cellCollection.GetSourceWordLabelMBOT(at1)->GetLabelMBOT().GetWord(at2)) << std::endl;
     return m_cellCollection.GetSourceWordLabelMBOT(at1)->GetLabelMBOT().GetWord(at2);
   }

  /** abstract function. Return a vector of translation options for given a range in the input sentence
   *  \param range source range for which you want the translation options
   *  \param outColl return argument
   */
  virtual void GetChartRuleCollection(
    const WordsRange &range,
    ChartParserCallback &outColl) = 0;

  //Fabienne Braune : Get an l-MBOT rule collection.
  virtual void GetMBOTRuleCollection(
    const WordsRange &range,
    ChartParserCallback &outColl) = 0;

private:
  //! Non-copyable: copy constructor and assignment operator not implemented.
  ChartRuleLookupManagerMBOT(const ChartRuleLookupManagerMBOT &);
  //! Non-copyable: copy constructor and assignment operator not implemented.
  ChartRuleLookupManagerMBOT &operator=(const ChartRuleLookupManagerMBOT &);

  const InputType &m_sentence;
  const ChartCellCollectionBase &m_cellCollection;
};

}  // namespace Moses

#endif
