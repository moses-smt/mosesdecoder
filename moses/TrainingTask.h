#pragma once

#include <boost/smart_ptr/shared_ptr.hpp>
#include "moses/ThreadPool.h"
#include "moses/TranslationOptionCollection.h"
#include "moses/IOWrapper.h"

namespace Moses
{
class InputType;
class OutputCollector;


class TrainingTask : public Moses::Task
{

public:

  TrainingTask(Moses::InputType* source, Moses::IOWrapper &ioWrapper)
  : m_source(source)
    , m_ioWrapper(ioWrapper)
  {}

  ~TrainingTask()
  {}

  void Run()
  {
    StaticData::Instance().InitializeForInput(*m_source);
    
    std::cerr << *m_source << std::endl;
    
    TranslationOptionCollection *transOptColl = m_source->CreateTranslationOptionCollection();
    transOptColl->CreateTranslationOptions();
    delete transOptColl;
    
    StaticData::Instance().CleanUpAfterSentenceProcessing(*m_source);
  }


private:
  Moses::InputType* m_source;
  Moses::IOWrapper &m_ioWrapper;

};


} //namespace
