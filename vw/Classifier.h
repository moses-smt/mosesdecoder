#ifndef moses_Classifier_h
#define moses_Classifier_h

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <deque>
#include <vector>
#include <boost/shared_ptr.hpp>

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
typedef std::pair<uint32_t, float> FeatureType; // feature hash (=ID) and value
typedef std::vector<FeatureType> FeatureVector;

/**
* Abstract class to be implemented by classifiers.
*/
class Classifier
{
public:
  /**
   * Add a feature that does not depend on the class (label).
   */
  virtual FeatureType AddLabelIndependentFeature(const StringPiece &name, float value) = 0;

  /**
   * Add a feature that is specific for the given class.
   */
  virtual FeatureType AddLabelDependentFeature(const StringPiece &name, float value) = 0;

  /**
   * Efficient addition of features when their IDs are already computed.
   */
  virtual void AddLabelIndependentFeatureVector(const FeatureVector &features) = 0;

  /**
   * Efficient addition of features when their IDs are already computed.
   */
  virtual void AddLabelDependentFeatureVector(const FeatureVector &features) = 0;

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
  FeatureType AddLabelIndependentFeature(const StringPiece &name) {
    return AddLabelIndependentFeature(name, 1.0);
  }

  FeatureType AddLabelDependentFeature(const StringPiece &name) {
    return AddLabelDependentFeature(name, 1.0);
  }

  virtual ~Classifier() {}

protected:
  /**
   * Escape special characters in a unified way.
   */
  static std::string EscapeSpecialChars(const std::string &str) {
    std::string out;
    out = Moses::Replace(str, "\\", "_/_");
    out = Moses::Replace(out, "|", "\\/");
    out = Moses::Replace(out, ":", "\\;");
    out = Moses::Replace(out, " ", "\\_");
    return out;
  }

  const static bool DEBUG = false;

};

// some of VW settings are hard-coded because they are always needed in our scenario
// (e.g. quadratic source X target features)
const std::string VW_DEFAULT_OPTIONS = " --hash all --noconstant -q st -t --ldf_override sc ";
const std::string VW_DEFAULT_PARSER_OPTIONS = " --quiet --hash all --noconstant -q st -t --csoaa_ldf sc ";

/**
 * Produce VW training file (does not use the VW library!)
 */
class VWTrainer : public Classifier
{
public:
  VWTrainer(const std::string &outputFile);
  virtual ~VWTrainer();

  virtual FeatureType AddLabelIndependentFeature(const StringPiece &name, float value);
  virtual FeatureType AddLabelDependentFeature(const StringPiece &name, float value);
  virtual void AddLabelIndependentFeatureVector(const FeatureVector &features);
  virtual void AddLabelDependentFeatureVector(const FeatureVector &features);
  virtual void Train(const StringPiece &label, float loss);
  virtual float Predict(const StringPiece &label);

protected:
  void AddFeature(const StringPiece &name, float value);

  bool m_isFirstSource, m_isFirstTarget, m_isFirstExample;

private:
  boost::iostreams::filtering_ostream m_bfos;
  std::deque<std::string> m_outputBuffer;

  void WriteBuffer();
};

/**
 * Predict using VW library.
 */
class VWPredictor : public Classifier, private boost::noncopyable
{
public:
  VWPredictor(const std::string &modelFile, const std::string &vwOptions);
  virtual ~VWPredictor();

  virtual FeatureType AddLabelIndependentFeature(const StringPiece &name, float value);
  virtual FeatureType AddLabelDependentFeature(const StringPiece &name, float value);
  virtual void AddLabelIndependentFeatureVector(const FeatureVector &features);
  virtual void AddLabelDependentFeatureVector(const FeatureVector &features);
  virtual void Train(const StringPiece &label, float loss);
  virtual float Predict(const StringPiece &label);

  friend class ClassifierFactory;

protected:
  FeatureType AddFeature(const StringPiece &name, float values);

  ::vw *m_VWInstance, *m_VWParser;
  ::ezexample *m_ex;
  // if true, then the VW instance is owned by an external party and should NOT be
  // deleted at end; if false, then we own the VW instance and must clean up after it.
  bool m_sharedVwInstance;
  bool m_isFirstSource, m_isFirstTarget;

private:
  // instantiation by classifier factory
  VWPredictor(vw * instance, const std::string &vwOption);
};

/**
 * Provider for classifier instances to be used by individual threads.
 */
class ClassifierFactory : private boost::noncopyable
{
public:
  typedef boost::shared_ptr<Classifier> ClassifierPtr;

  /**
   * Creates VWPredictor instances to be used by individual threads.
   */
  ClassifierFactory(const std::string &modelFile, const std::string &vwOptions);

  /**
   * Creates VWTrainer instances (which write features to a file).
   */
  ClassifierFactory(const std::string &modelFilePrefix);

  // return VWPredictor or VWTrainer instance depending on whether we're in training mode
  ClassifierPtr operator()();

  ~ClassifierFactory();

private:
  std::string m_vwOptions;
  ::vw *m_VWInstance;
  int m_lastId;
  std::string m_modelFilePrefix;
  bool m_gzip;
  boost::mutex m_mutex;
  const bool m_train;
};

} // namespace Discriminative

#endif // moses_Classifier_h
