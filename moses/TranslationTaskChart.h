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
class TranslationTaskChart : public Moses::Task
{
public:
  TranslationTaskChart(Moses::InputType *source, MosesChartCmd::IOWrapperChart &ioWrapper,
    boost::shared_ptr<Moses::HypergraphOutput<Moses::ChartManager> > hypergraphOutput);

  ~TranslationTaskChart();

  void Run();

private:
  // Non-copyable: copy constructor and assignment operator not implemented.
  TranslationTaskChart(const TranslationTaskChart &);
  TranslationTaskChart &operator=(const TranslationTaskChart &);

  Moses::InputType *m_source;
  MosesChartCmd::IOWrapperChart &m_ioWrapper;
  boost::shared_ptr<Moses::HypergraphOutput<Moses::ChartManager> > m_hypergraphOutput;
};

