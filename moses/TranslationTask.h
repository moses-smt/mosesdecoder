#pragma once

#include <boost/smart_ptr/shared_ptr.hpp>
#include "moses/ThreadPool.h"
#include "moses/Manager.h"
#include "moses/HypergraphOutput.h"
#include "moses/Manager.h"
#include "moses/ChartManager.h"

namespace Moses
{
class InputType;
class OutputCollector;

class IOWrapper;

/** Translates a sentence.
  * - calls the search (Manager)
  * - applies the decision rule
  * - outputs best translation and additional reporting
  **/
class TranslationTask : public Moses::Task
{

public:

  TranslationTask(Moses::InputType* source, Moses::IOWrapper &ioWrapper,
                  bool outputSearchGraphSLF,
                  boost::shared_ptr<Moses::HypergraphOutput<Moses::Manager> > hypergraphOutput);

  TranslationTask(Moses::InputType *source, IOWrapper &ioWrapper,
    boost::shared_ptr<Moses::HypergraphOutput<Moses::ChartManager> > hypergraphOutputChart);

  ~TranslationTask();

  /** Translate one sentence
   * gets called by main function implemented at end of this source file */
  void Run();


private:
  int m_pbOrChart; // 1=pb. 2=chart
  Moses::InputType* m_source;
  Moses::IOWrapper &m_ioWrapper;

  bool m_outputSearchGraphSLF;
  boost::shared_ptr<Moses::HypergraphOutput<Moses::Manager> > m_hypergraphOutput;
  boost::shared_ptr<Moses::HypergraphOutput<Moses::ChartManager> > m_hypergraphOutputChart;

  void RunPb();
  void RunChart();

};


} //namespace
