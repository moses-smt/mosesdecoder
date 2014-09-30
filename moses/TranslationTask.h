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

/** Translates a sentence.
  * - calls the search (Manager)
  * - applies the decision rule
  * - outputs best translation and additional reporting
  **/
class TranslationTask : public Moses::Task
{

public:

  TranslationTask(size_t lineNumber,
                  Moses::InputType* source, Moses::OutputCollector* outputCollector, Moses::OutputCollector* nbestCollector,
                  Moses::OutputCollector* latticeSamplesCollector,
                  Moses::OutputCollector* wordGraphCollector, Moses::OutputCollector* searchGraphCollector,
                  Moses::OutputCollector* detailedTranslationCollector,
                  Moses::OutputCollector* alignmentInfoCollector,
                  Moses::OutputCollector* unknownsCollector,
                  bool outputSearchGraphSLF,
                  boost::shared_ptr<Moses::HypergraphOutput<Moses::Manager> > hypergraphOutput);

  ~TranslationTask();

  /** Translate one sentence
   * gets called by main function implemented at end of this source file */
  void Run();


private:
  Moses::InputType* m_source;
  size_t m_lineNumber;
  Moses::OutputCollector* m_outputCollector;
  Moses::OutputCollector* m_nbestCollector;
  Moses::OutputCollector* m_latticeSamplesCollector;
  Moses::OutputCollector* m_wordGraphCollector;
  Moses::OutputCollector* m_searchGraphCollector;
  Moses::OutputCollector* m_detailedTranslationCollector;
  Moses::OutputCollector* m_alignmentInfoCollector;
  Moses::OutputCollector* m_unknownsCollector;
  bool m_outputSearchGraphSLF;
  boost::shared_ptr<Moses::HypergraphOutput<Moses::Manager> > m_hypergraphOutput;
  std::ofstream *m_alignmentStream;


};


} //namespace
