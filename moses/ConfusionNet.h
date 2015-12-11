// $Id$

#ifndef moses_ConfusionNet_h
#define moses_ConfusionNet_h

#include <vector>
#include <iostream>
#include "Word.h"
#include "InputType.h"
#include "NonTerminal.h"
#include "util/exception.hh"

namespace Moses
{

class FactorCollection;
class TranslationOptionCollection;
class Sentence;
class TranslationTask;

/** An input to the decoder where each position can be 1 of a number of words,
 *  each with an associated probability. Compared with a sentence, where each position is a word
 */
class ConfusionNet : public InputType
{
public:
  typedef std::vector<std::pair<Word, ScorePair > > Column;

protected:
  std::vector<Column> data;
  NonTerminalSet m_defaultLabelSet;

  bool ReadFormat0(std::istream&);
  bool ReadFormat1(std::istream&);
  void String2Word(const std::string& s,Word& w,const std::vector<FactorType>& factorOrder);

public:
  ConfusionNet(AllOptions::ptr const& opts);
  virtual ~ConfusionNet();

  ConfusionNet(Sentence const& s);

  InputTypeEnum GetType() const {
    return ConfusionNetworkInput;
  }

  const Column& GetColumn(size_t i) const {
    UTIL_THROW_IF2(i >= data.size(),
                   "Out of bounds. Trying to access " << i
                   << " when vector only contains " << data.size());
    return data[i];
  }
  const Column& operator[](size_t i) const {
    return GetColumn(i);
  }
  virtual size_t GetColumnIncrement(size_t i, size_t j) const; //! returns 1 for CNs

  bool Empty() const {
    return data.empty();
  }
  size_t GetSize() const {
    return data.size();
  }
  void Clear() {
    data.clear();
  }

  bool ReadF(std::istream&, int format=0);
  virtual void Print(std::ostream&) const;

  int Read(std::istream& in);

  Phrase GetSubString(const Range&) const; //TODO not defined
  std::string GetStringRep(const std::vector<FactorType> factorsToPrint) const; //TODO not defined
  const Word& GetWord(size_t pos) const;

  TranslationOptionCollection*
  CreateTranslationOptionCollection(ttasksptr const& ttask) const;

  const NonTerminalSet &GetLabelSet(size_t /*startPos*/, size_t /*endPos*/) const {
    return m_defaultLabelSet;
  }


};

std::ostream& operator<<(std::ostream& out,const ConfusionNet& cn);


}

#endif
