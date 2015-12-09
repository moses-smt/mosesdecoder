// -*- c++ -*-
#pragma once

#include <iostream>
#include <string>
#include "ScoreComponentCollection.h"
#include "InputType.h"
#include "moses/parameters/AllOptions.h"
namespace Moses
{
class ScoreComponentCollection;
class FeatureFunction;
class OutputCollector;

class BaseManager
{
protected:
  // const InputType &m_source; /**< source sentence to be translated */
  ttaskwptr m_ttask;
  InputType const& m_source;

  BaseManager(ttasksptr const& ttask);

  // output
  typedef std::vector<std::pair<Moses::Word, Moses::Range> > ApplicationContext;
  typedef std::set< std::pair<size_t, size_t>  > Alignments;

  void OutputSurface(std::ostream &out, Phrase const& phrase) const;

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
  virtual ~BaseManager() { }

  //! the input sentence being decoded
  const InputType& GetSource() const;
  const ttasksptr  GetTtask() const;
  AllOptions::ptr const& options() const;

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
  // virtual void OutputSearchGraphHypergraph() const = 0;

  virtual void OutputSearchGraphAsHypergraph(std::ostream& out) const;
  virtual void OutputSearchGraphAsHypergraph(std::string const& fname,
      size_t const precision) const;
  /***
   * to be called after processing a sentence
   */
  virtual void CalcDecoderStatistics() const = 0;

};

}
