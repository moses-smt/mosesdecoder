#pragma once

#include <boost/smart_ptr/shared_ptr.hpp>
#include "moses/ThreadPool.h"
#include "moses/Manager.h"
#include "moses/HypergraphOutput.h"
#include "moses/IOWrapper.h"
#include "moses/Manager.h"
#include "moses/ChartManager.h"

#include "moses/Syntax/S2T/Manager.h"

namespace Moses
{
class InputType;
class OutputCollector;


/** Translates a sentence.
  * - calls the search (Manager)
  * - applies the decision rule
  * - outputs best translation and additional reporting
  **/
class TranslationTask : public Moses::Task
{

public:

  TranslationTask(Moses::InputType* source, Moses::IOWrapper &ioWrapper, int pbOrChart);

  ~TranslationTask();

  /** Translate one sentence
   * gets called by main function implemented at end of this source file */
  void Run();


private:
  int m_pbOrChart; // 1=pb. 2=chart
  Moses::InputType* m_source;
  Moses::IOWrapper &m_ioWrapper;

  void RunPb();
  void RunChart();


  template<typename Parser>
  void DecodeS2T() {
    const StaticData &staticData = StaticData::Instance();
    const std::size_t translationId = m_source->GetTranslationId();
    Syntax::S2T::Manager<Parser> manager(*m_source);
    manager.Decode();
    // 1-best
    manager.OutputBest(m_ioWrapper.GetSingleBestOutputCollector());

    // n-best
    manager.OutputNBest(m_ioWrapper.GetNBestOutputCollector());

    // Write 1-best derivation (-translation-details / -T option).

    manager.OutputDetailedTranslationReport(m_ioWrapper.GetDetailedTranslationCollector());

    manager.OutputUnknowns(m_ioWrapper.GetUnknownsCollector());
  }

};


} //namespace
