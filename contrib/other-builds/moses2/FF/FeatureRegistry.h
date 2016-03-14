#pragma once
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

namespace Moses2
{

////////////////////////////////////////////////////////////////////
class FeatureFactory
{
public:
  virtual ~FeatureFactory() {}

  virtual void Create(size_t startInd, const std::string &line) = 0;

protected:
  template <class F> static void DefaultSetup(F *feature);

  FeatureFactory() {}
};

template <class F>
void
FeatureFactory
::DefaultSetup(F *feature)
{

}

////////////////////////////////////////////////////////////////////
template <class F>
class DefaultFeatureFactory : public FeatureFactory
{
public:
  void Create(size_t startInd, const std::string &line) {
    DefaultSetup(new F(startInd, line));
  }
};

////////////////////////////////////////////////////////////////////
class FeatureRegistry
{
public:
  FeatureRegistry();

  ~FeatureRegistry();

  void Construct(const std::string &name, const std::string &line);
  void PrintFF() const;

private:
  void Add(const std::string &name, FeatureFactory *factory);

  typedef boost::unordered_map<std::string, boost::shared_ptr<FeatureFactory> > Map;

  Map registry_;
};

////////////////////////////////////////////////////////////////////


}

