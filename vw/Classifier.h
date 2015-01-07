#ifndef moses_Classifier_h
#define moses_Classifier_h

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <deque>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/iostreams/filtering_stream.hpp> 
#include <boost/iostreams/filter/gzip.hpp>
#include "../util/string_piece.hh"
#include "../moses/Util.h"

// forward declarations to avoid dependency on VW 
struct vw;
class ezexample;

namespace Discriminative
{

/**
* Abstract class to be implemented by classifiers.
*/
class Classifier
{
public:
  /**
   * Add a feature that does not depend on the class (label).
   */
  virtual void AddLabelIndependentFeature(const StringPiece &name, float value) = 0;

  /**
   * Add a feature that is specific for the given class.
   */
  virtual void AddLabelDependentFeature(const StringPiece &name, float value) = 0;

  /**
   * Train using current example. Use loss to distinguish positive and negative training examples.
   * Throws away current label-dependent features (so that features for another label/class can now be set).
   */
  virtual void Train(const StringPiece &label, float loss) = 0;
  
  /**
   * Predict the loss (inverse of score) of current example.
   * Throws away current label-dependent features (so that features for another label/class can now be set).
   */
  virtual float Predict(const StringPiece &label) = 0;

  // helper methods for indicator features
  void AddLabelIndependentFeature(const StringPiece &name)
  {
    AddLabelIndependentFeature(name, 1.0);
  }

  void AddLabelDependentFeature(const StringPiece &name)
  {
    AddLabelDependentFeature(name, 1.0);
  }

  /**
   * Escape special characters in a unified way.
   */
  static std::string EscapeSpecialChars(const std::string &str)
  {
    std::string out;
    out = Moses::Replace(str, "\\", "\\\\");
    out = Moses::Replace(out, "|", "\\/");
    out = Moses::Replace(out, ":", "\\;");
    out = Moses::Replace(out, " ", "\\_");
    return out;
  }
};

// some of VW settings are hard-coded because they are always needed in our scenario
// (e.g. quadratic source X target features)
const std::string VW_DEFAULT_OPTIONS = " --hash all --noconstant -q st -t --csoaa_ldf s ";

/** 
 * Produce VW training file (does not use the VW library!)
 */
class VWTrainer : public Classifier
{
public:
  VWTrainer(const std::string &outputFile);

  virtual void AddLabelIndependentFeature(const StringPiece &name, float value);
  virtual void AddLabelDependentFeature(const StringPiece &name, float value);
  virtual void Train(const StringPiece &label, float loss);
  virtual float Predict(const StringPiece &label);

protected:
  void AddFeature(const StringPiece &name, float value);
  void Finish();

  bool m_isFirstSource, m_isFirstTarget, m_isFirstExample;

private:
  boost::iostreams::filtering_ostream m_bfos;
  std::deque<std::string> m_outputBuffer;

  void WriteBuffer();
  std::string EscapeSpecialChars(const std::string &str);
};

/**
 * Predict using VW library.
 */
class VWPredictor : public Classifier, private boost::noncopyable
{
public:
  VWPredictor(const std::string &modelFile, const std::string &vwOptions);

  virtual void AddLabelIndependentFeature(const StringPiece &name, float value);
  virtual void AddLabelDependentFeature(const StringPiece &name, float value);
  virtual void Train(const StringPiece &label, float loss);
  virtual float Predict(const StringPiece &label);

  friend class VWPredictorFactory;

protected:
  void AddFeature(const StringPiece &name, float value);
  void Finish();

  ::vw *m_VWInstance;
  ::ezexample *m_ex;
  // if true, then the VW instance is owned by an external party and should NOT be
  // deleted at end; if false, then we own the VW instance and must clean up after it.
  bool m_sharedVwInstance;
  int m_index;
  bool m_isFirstSource, m_isFirstTarget;

  ~VWPredictor();

private:
  VWPredictor(vw * instance, int index); // instantiation by VWPredictorFactory
};
  
/**
  * Object pool of VWPredictors.
  */
class VWPredictorFactory : private boost::noncopyable
{
public:
  VWPredictorFactory(const std::string &modelFile, const std::string &vwOptions, const int poolSize = DEFAULT_POOL_SIZE);

  /**
  * Get an instance of VWPredictor from the pool.
  */
  VWPredictor * Acquire();

  /**
  * Release a VWPredictor instance.
  */
  void Release(VWPredictor *vwpred);

  ~VWPredictorFactory();

private:
  ::vw *m_VWInstance;
  int m_firstFree;
  std::vector<int> m_nextFree;
  std::vector<VWPredictor *> m_predictors;
  boost::mutex m_mutex;
  boost::condition_variable m_cond;

  const static int DEFAULT_POOL_SIZE = 32;
};

} // namespace Discriminative

#endif // moses_Classifier_h
