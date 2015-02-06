#include "DomainFeature.h"
#include "ExtractionPhrasePair.h"
#include "tables-core.h"
#include "InputFileStream.h"

using namespace std;

namespace MosesTraining
{

// handling of domain names: load database with sentence-id / domain name info
void Domain::load( const std::string &domainFileName )
{
  Moses::InputFileStream fileS( domainFileName );
  istream *fileP = &fileS;

  string line;
  while(getline(*fileP, line)) {
    // read
    vector< string > domainSpecLine = tokenize( line.c_str() );
    int lineNumber;
    if (domainSpecLine.size() != 2 ||
        ! sscanf(domainSpecLine[0].c_str(), "%d", &lineNumber)) {
      std::cerr << "ERROR: in domain specification line: '" << line << "'" << endl;
      exit(1);
    }
    // store
    string &name = domainSpecLine[1];
    spec.push_back( make_pair( lineNumber, name ));
    if (name2id.find( name ) == name2id.end()) {
      name2id[ name ] = list.size();
      list.push_back( name );
    }
  }
}

// get domain name based on sentence number
string Domain::getDomainOfSentence( int sentenceId ) const
{
  for(size_t i=0; i<spec.size(); i++) {
    if (sentenceId <= spec[i].first) {
      return spec[i].second;
    }
  }
  return "undefined";
}

DomainFeature::DomainFeature(const string& domainFile) : m_propertyKey("domain")
{
  //process domain file
  m_domain.load(domainFile);
}

void DomainFeature::addPropertiesToPhrasePair(ExtractionPhrasePair &phrasePair,
    float count,
    int sentenceId) const
{
  std::string value = m_domain.getDomainOfSentence(sentenceId);
  phrasePair.AddProperty(m_propertyKey, value, count);
}

void DomainFeature::add(const ScoreFeatureContext& context,
                        std::vector<float>& denseValues,
                        std::map<std::string,float>& sparseValues)  const
{
  const map<string,float> *domainCount = context.phrasePair.GetProperty(m_propertyKey);
  assert( domainCount != NULL );
  add(*domainCount,
      context.phrasePair.GetCount(),
      context.maybeLog,
      denseValues, sparseValues);
}

void SubsetDomainFeature::add(const map<string,float>& domainCount,
                              float count,
                              const MaybeLog& maybeLog,
                              std::vector<float>& denseValues,
                              std::map<std::string,float>& sparseValues)  const
{
  if (m_domain.list.size() > 6) {
    UTIL_THROW_IF(m_domain.list.size() > 6, ScoreFeatureArgumentException,
                  "too many domains for core domain subset features");
  }
  size_t bitmap = 0;
  for(size_t bit = 0; bit < m_domain.list.size(); bit++) {
    if (domainCount.find( m_domain.list[ bit ] ) != domainCount.end()) {
      bitmap += 1 << bit;
    }
  }
  for(size_t i = 1; i < (1 << m_domain.list.size()); i++) {
    denseValues.push_back(maybeLog( (bitmap == i) ? 2.718 : 1 ));
  }
}

void SparseSubsetDomainFeature::add(const map<string,float>& domainCount,float count,
                                    const MaybeLog& maybeLog,
                                    std::vector<float>& denseValues,
                                    std::map<std::string,float>& sparseValues)  const
{
  typedef vector<string>::const_iterator I;
  ostringstream key;
  key << "doms";
  for (I i = m_domain.list.begin(); i != m_domain.list.end(); ++i) {
    if (domainCount.find(*i) != domainCount.end()) {
      key << "_" << *i;
    }
  }
  sparseValues[key.str()] = 1;
}


void RatioDomainFeature::add(const map<string,float>& domainCount,float count,
                             const MaybeLog& maybeLog,
                             std::vector<float>& denseValues,
                             std::map<std::string,float>& sparseValues)  const
{
  typedef vector< string >::const_iterator I;
  for (I i = m_domain.list.begin(); i != m_domain.list.end(); i++ ) {
    map<string,float>::const_iterator dci = domainCount.find(*i);
    if (dci == domainCount.end() ) {
      denseValues.push_back(maybeLog( 1 ));
    } else {
      denseValues.push_back(maybeLog(exp( dci->second / count ) ));
    }
  }
}


void SparseRatioDomainFeature::add(const map<string,float>& domainCount,float count,
                                   const MaybeLog& maybeLog,
                                   std::vector<float>& denseValues,
                                   std::map<std::string,float>& sparseValues)  const
{
  typedef map< string, float >::const_iterator I;
  for (I i=domainCount.begin(); i != domainCount.end(); i++) {
    sparseValues["domr_" + i->first] =  (i->second / count);
  }
}


void IndicatorDomainFeature::add(const map<string,float>& domainCount,float count,
                                 const MaybeLog& maybeLog,
                                 std::vector<float>& denseValues,
                                 std::map<std::string,float>& sparseValues)  const
{
  typedef vector< string >::const_iterator I;
  for (I i = m_domain.list.begin(); i != m_domain.list.end(); i++ ) {
    map<string,float>::const_iterator dci = domainCount.find(*i);
    if (dci == domainCount.end() ) {
      denseValues.push_back(maybeLog( 1 ));
    } else {
      denseValues.push_back(maybeLog(2.718));
    }
  }
}

void SparseIndicatorDomainFeature::add(const map<string,float>& domainCount,float count,
                                       const MaybeLog& maybeLog,
                                       std::vector<float>& denseValues,
                                       std::map<std::string,float>& sparseValues)  const
{
  typedef map< string, float >::const_iterator I;
  for (I i=domainCount.begin(); i != domainCount.end(); i++) {
    sparseValues["dom_" + i->first] = 1;
  }
}

}

