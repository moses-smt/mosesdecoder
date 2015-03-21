// -*- c++ -*-
#pragma once

#include <boost/smart_ptr/shared_ptr.hpp>
#include "moses/ThreadPool.h"
#include "moses/Manager.h"
#include "moses/HypergraphOutput.h"
#include "moses/IOWrapper.h"
#include "moses/Manager.h"
#include "moses/ChartManager.h"

#include "moses/Syntax/F2S/Manager.h"
#include "moses/Syntax/S2T/Manager.h"
#include "moses/Syntax/T2S/Manager.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>

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
  // no copying, no assignment 
  TranslationTask(TranslationTask const& other) { }

  TranslationTask const& 
  operator=(TranslationTask const& other) { return *this; }

protected:
  boost::weak_ptr<TranslationTask> m_self; // weak ptr to myself

  TranslationTask() { } ;
  TranslationTask(boost::shared_ptr<Moses::InputType> const& source, 
		  boost::shared_ptr<Moses::IOWrapper> const& ioWrapper);
  // Yes, the constructor is protected. 
  //
  // TranslationTasks can only be created through the creator
  // functions create(...). The creator functions set m_self to a
  // weak_pointer s.t m_self.get() == this. The public member function
  // self() can then be used to get a shared_ptr to the Task that
  // guarantees the existence of the Task while that pointer is live.
  // Depending on the use, case, that shared pointer can be kept alive
  // or copied into a weak pointer that can then be used e.g. as a
  // hash key for caching context-dependent information in feature
  // functions.  When it is time to clean up the cache, the feature
  // function can determine (via a check on the weak pointer) if the
  // task is still live or not, or maintain a shared_ptr to ensure the
  // task stays alive till it's done with it.

public:

  virtual
  boost::shared_ptr<TranslationTask>  
  self() { return m_self.lock(); }

  virtual
  boost::shared_ptr<TranslationTask const>  
  self() const { return m_self.lock(); }

  // creator functions
  static boost::shared_ptr<TranslationTask> create(); 

  static boost::shared_ptr<TranslationTask> 
  create(boost::shared_ptr<Moses::InputType> const& source, 
	 boost::shared_ptr<Moses::IOWrapper> const& ioWrapper);
  
  ~TranslationTask();
  /** Translate one sentence
   * gets called by main function implemented at end of this source file */
  virtual void Run();

private:
  boost::shared_ptr<Moses::InputType> m_source; 
  boost::shared_ptr<Moses::IOWrapper> m_ioWrapper;

};


} //namespace
