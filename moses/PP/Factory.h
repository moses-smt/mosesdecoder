#pragma once

#include "moses/PP/PhraseProperty.h"
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace Moses
{

class PhrasePropertyCreator;

class PhrasePropertyFactory
{
public:
  PhrasePropertyFactory();

  ~PhrasePropertyFactory();

  boost::shared_ptr<PhraseProperty> ProduceProperty(const std::string &key, const std::string &value) const;
  void PrintPP() const;

private:
  void Add(const std::string &name, PhrasePropertyCreator *creator);

  typedef boost::unordered_map<std::string, boost::shared_ptr<PhrasePropertyCreator> > Registry;

  Registry m_registry;
};

} // namespace Moses

