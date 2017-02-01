#pragma once
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

namespace Moses2
{
class FeatureFunction;

////////////////////////////////////////////////////////////////////
class FeatureFactory
{
public:
  virtual ~FeatureFactory() {
  }

  virtual FeatureFunction *Create(size_t startInd, const std::string &line) const = 0;

protected:
  FeatureFactory() {
  }
};

////////////////////////////////////////////////////////////////////
class FeatureRegistry
{
public:
  static const FeatureRegistry &Instance() {
    return s_instance;
  }

  ~FeatureRegistry();

  FeatureFunction *Construct(size_t startInd, const std::string &name,
                             const std::string &line) const;
  void PrintFF() const;

private:
  static FeatureRegistry s_instance;

  typedef boost::unordered_map<std::string, boost::shared_ptr<FeatureFactory> > Map;
  Map registry_;

  FeatureRegistry();

  void Add(const std::string &name, FeatureFactory *factory);

};

////////////////////////////////////////////////////////////////////

}

