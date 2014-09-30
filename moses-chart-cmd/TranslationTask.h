#pragma once

#include <boost/smart_ptr/shared_ptr.hpp>
#include "moses/ThreadPool.h"
#include "moses/ChartManager.h"
#include "moses/HypergraphOutput.h"

namespace Moses
{
	class InputType;
	class OutputCollector;
}

namespace MosesChartCmd
{
	class IOWrapperChart;
}

/**
  * Translates a sentence.
 **/
class TranslationTask : public Moses::Task
{
public:
  TranslationTask(Moses::InputType *source, MosesChartCmd::IOWrapperChart &ioWrapper,
    boost::shared_ptr<Moses::HypergraphOutput<Moses::ChartManager> > hypergraphOutput);

  ~TranslationTask();

  void Run();

private:
  // Non-copyable: copy constructor and assignment operator not implemented.
  TranslationTask(const TranslationTask &);
  TranslationTask &operator=(const TranslationTask &);

  Moses::InputType *m_source;
  MosesChartCmd::IOWrapperChart &m_ioWrapper;
  boost::shared_ptr<Moses::HypergraphOutput<Moses::ChartManager> > m_hypergraphOutput;
};

