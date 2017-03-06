//-*- c++ -*-
#pragma once

#include <boost/smart_ptr/shared_ptr.hpp>
#include "moses/ThreadPool.h"
#include "moses/TranslationOptionCollection.h"
#include "moses/IOWrapper.h"
#include "moses/TranslationTask.h"

namespace Moses
{
class InputType;
class OutputCollector;


class TrainingTask : public Moses::TranslationTask
{

protected:
  TrainingTask(boost::shared_ptr<Moses::InputType> const source,
               boost::shared_ptr<Moses::IOWrapper> const ioWrapper)
    : TranslationTask(source, ioWrapper)
  { }

public:

  // factory function
  static boost::shared_ptr<TrainingTask>
  create(boost::shared_ptr<InputType> const& source) {
    boost::shared_ptr<IOWrapper> nix;
    boost::shared_ptr<TrainingTask> ret(new TrainingTask(source, nix));
    ret->m_self = ret;
    return ret;
  }

  // factory function
  static boost::shared_ptr<TrainingTask>
  create(boost::shared_ptr<InputType> const& source,
         boost::shared_ptr<IOWrapper> const& ioWrapper) {
    boost::shared_ptr<TrainingTask> ret(new TrainingTask(source, ioWrapper));
    ret->m_self = ret;
    ret->m_scope.reset(new ContextScope);
    return ret;
  }

  // factory function
  static boost::shared_ptr<TrainingTask>
  create(boost::shared_ptr<InputType> const& source,
         boost::shared_ptr<IOWrapper> const& ioWrapper,
         boost::shared_ptr<ContextScope> const& scope) {
    boost::shared_ptr<TrainingTask> ret(new TrainingTask(source, ioWrapper));
    ret->m_self = ret;
    ret->m_scope = scope;
    return ret;
  }

  ~TrainingTask()
  { }

  void Run() {
    StaticData::Instance().InitializeForInput(this->self());

    std::cerr << *m_source << std::endl;

    TranslationOptionCollection *transOptColl
    = m_source->CreateTranslationOptionCollection(this->self());
    transOptColl->CreateTranslationOptions();
    delete transOptColl;

    StaticData::Instance().CleanUpAfterSentenceProcessing(this->self());
  }


private:
  // Moses::InputType* m_source;
  // Moses::IOWrapper &m_ioWrapper;

};


} //namespace
