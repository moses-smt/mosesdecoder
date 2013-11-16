#pragma once

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace Moses
{

class FeatureFactory;

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

} // namespace Moses
