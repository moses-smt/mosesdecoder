#pragma once

#include <boost/smart_ptr/shared_ptr.hpp>
#include "moses/ThreadPool.h"
#include "moses/Manager.h"
#include "moses/HypergraphOutput.h"

namespace Moses
{
	class InputType;
	class OutputCollector;
}

namespace MosesCmd
{

class IOWrapper;

/** Translates a sentence.
  * - calls the search (Manager)
  * - applies the decision rule
  * - outputs best translation and additional reporting
  **/
class TranslationTask : public Moses::Task
{

public:

  TranslationTask(size_t lineNumber, Moses::InputType* source, MosesCmd::IOWrapper &ioWrapper,
                  Moses::OutputCollector* latticeSamplesCollector,
                  bool outputSearchGraphSLF,
                  boost::shared_ptr<Moses::HypergraphOutput<Moses::Manager> > hypergraphOutput);

  ~TranslationTask();

  /** Translate one sentence
   * gets called by main function implemented at end of this source file */
  void Run();


private:
  Moses::InputType* m_source;
  size_t m_lineNumber;
  MosesCmd::IOWrapper &m_ioWrapper;

  Moses::OutputCollector* m_latticeSamplesCollector;
  bool m_outputSearchGraphSLF;
  boost::shared_ptr<Moses::HypergraphOutput<Moses::Manager> > m_hypergraphOutput;


};


} //namespace
