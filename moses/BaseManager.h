#pragma once

#include <iostream>
#include <string>
#include "ScoreComponentCollection.h"
#include "InputType.h"

namespace Moses
{
class ScoreComponentCollection;
class FeatureFunction;
class OutputCollector;

class BaseManager
{
protected:
  const InputType &m_source; /**< source sentence to be translated */

  BaseManager(const InputType &source)
    :m_source(source) {
  }

  // output
  typedef std::vector<std::pair<Moses::Word, Moses::WordsRange> > ApplicationContext;
  typedef std::set< std::pair<size_t, size_t>  > Alignments;

  void OutputSurface(std::ostream &out,
                     const Phrase &phrase,
                     const std::vector<FactorType> &outputFactorOrder,
                     bool reportAllFactors) const;
  void WriteApplicationContext(std::ostream &out,
                               const ApplicationContext &context) const;

  template <class T>
  void ShiftOffsets(std::vector<T> &offsets, T shift) const {
    T currPos = shift;
    for (size_t i = 0; i < offsets.size(); ++i) {
      if (offsets[i] == 0) {
        offsets[i] = currPos;
        ++currPos;
      } else {
        currPos += offsets[i];
      }
    }
  }

public:
  virtual ~BaseManager() {
  }

  //! the input sentence being decoded
  const InputType& GetSource() const {
    return m_source;
  }

  virtual void Decode() = 0;
  // outputs
  virtual void OutputBest(OutputCollector *collector) const = 0;
  virtual void OutputNBest(OutputCollector *collector) const = 0;
  virtual void OutputLatticeSamples(OutputCollector *collector) const = 0;
  virtual void OutputAlignment(OutputCollector *collector) const = 0;
  virtual void OutputDetailedTranslationReport(OutputCollector *collector) const = 0;
  virtual void OutputDetailedTreeFragmentsTranslationReport(OutputCollector *collector) const = 0;
  virtual void OutputWordGraph(OutputCollector *collector) const = 0;
  virtual void OutputSearchGraph(OutputCollector *collector) const = 0;
  virtual void OutputUnknowns(OutputCollector *collector) const = 0;
  virtual void OutputSearchGraphSLF() const = 0;
  virtual void OutputSearchGraphHypergraph() const = 0;

  /***
   * to be called after processing a sentence
   */
  virtual void CalcDecoderStatistics() const = 0;

};

}
