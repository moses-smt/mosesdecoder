
#include "moses/PP/Factory.h"
#include "util/exception.hh"
#include <iostream>
#include <vector>

#include "moses/PP/CountsPhraseProperty.h"
#include "moses/PP/SourceLabelsPhraseProperty.h"
#include "moses/PP/TargetPreferencesPhraseProperty.h"
#include "moses/PP/TreeStructurePhraseProperty.h"
#include "moses/PP/SpanLengthPhraseProperty.h"
#include "moses/PP/NonTermContextProperty.h"
#include "moses/PP/OrientationPhraseProperty.h"
#include "moses/PP/TargetConstituentBoundariesLeftPhraseProperty.h"
#include "moses/PP/TargetConstituentBoundariesRightAdjacentPhraseProperty.h"

namespace Moses
{

class PhrasePropertyCreator
{
public:
  virtual ~PhrasePropertyCreator() {}

  virtual boost::shared_ptr<PhraseProperty> CreateProperty(const std::string &value) = 0;

protected:
  template <class P> boost::shared_ptr<P> Create(P *property);

  PhrasePropertyCreator() {}
};

template <class P> boost::shared_ptr<P> PhrasePropertyCreator::Create(P *property)
{
  return boost::shared_ptr<P>(property);
}

namespace
{

template <class P> class DefaultPhrasePropertyCreator : public PhrasePropertyCreator
{
public:
  boost::shared_ptr<PhraseProperty> CreateProperty(const std::string &value) {
    P* property = new P();
    property->ProcessValue(value);
    return Create(property);
  }
};

} // namespace


PhrasePropertyFactory::PhrasePropertyFactory()
{
// Property with same key as class
#define MOSES_PNAME(name) Add(#name, new DefaultPhrasePropertyCreator< name >());
// Properties with different key than class.
#define MOSES_PNAME2(name, type) Add(name, new DefaultPhrasePropertyCreator< type >());

  MOSES_PNAME2("Counts", CountsPhraseProperty);
  MOSES_PNAME2("SourceLabels", SourceLabelsPhraseProperty);
  MOSES_PNAME2("TargetConstituentBoundariesLeft", TargetConstituentBoundariesLeftPhraseProperty);
  MOSES_PNAME2("TargetConstituentBoundariesRightAdjacent", TargetConstituentBoundariesRightAdjacentPhraseProperty);
  MOSES_PNAME2("TargetPreferences", TargetPreferencesPhraseProperty);
  MOSES_PNAME2("Tree",TreeStructurePhraseProperty);
  MOSES_PNAME2("SpanLength", SpanLengthPhraseProperty);
  MOSES_PNAME2("NonTermContext", NonTermContextProperty);
  MOSES_PNAME2("Orientation", OrientationPhraseProperty);
}

PhrasePropertyFactory::~PhrasePropertyFactory()
{
}

void PhrasePropertyFactory::Add(const std::string &name, PhrasePropertyCreator *creator)
{
  std::pair<std::string, boost::shared_ptr<PhrasePropertyCreator> > to_ins(name, boost::shared_ptr<PhrasePropertyCreator>(creator));
  UTIL_THROW_IF2(!m_registry.insert(to_ins).second, "Phrase property registered twice: " << name);
}

namespace
{
class UnknownPhrasePropertyException : public util::Exception {};
}

boost::shared_ptr<PhraseProperty> PhrasePropertyFactory::ProduceProperty(const std::string &key, const std::string &value) const
{
  Registry::const_iterator i = m_registry.find(key);
  UTIL_THROW_IF(i == m_registry.end(), UnknownPhrasePropertyException, "Phrase property is not registered: " << key);
  return i->second->CreateProperty(value);
}

void PhrasePropertyFactory::PrintPP() const
{
  std::cerr << "Registered phrase properties:" << std::endl;
  Registry::const_iterator iter;
  for (iter = m_registry.begin(); iter != m_registry.end(); ++iter) {
    const std::string &ppName = iter->first;
    std::cerr << ppName << " ";
  }
  std::cerr << std::endl;
}

} // namespace Moses

