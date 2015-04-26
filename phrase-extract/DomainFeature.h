// $Id$

#ifndef _DOMAIN_H
#define _DOMAIN_H

#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <string>
#include <queue>
#include <map>
#include <cmath>

#include "ScoreFeature.h"

namespace MosesTraining
{

class Domain
{
public:
  std::vector< std::pair< int, std::string > > spec;
  std::vector< std::string > list;
  std::map< std::string, int > name2id;
  void load( const std::string &fileName );
  std::string getDomainOfSentence( int sentenceId ) const;
};

class DomainFeature : public ScoreFeature
{
public:

  DomainFeature(const std::string& domainFile);

  void addPropertiesToPhrasePair(ExtractionPhrasePair &phrasePair,
                                 float count,
                                 int sentenceId) const;

  void add(const ScoreFeatureContext& context,
           std::vector<float>& denseValues,
           std::map<std::string,float>& sparseValues) const;

protected:
  /** Overridden in subclass */
  virtual void add(const std::map<std::string,float>& domainCounts, float count,
                   const MaybeLog& maybeLog,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const = 0;


  Domain m_domain;

  const std::string m_propertyKey;

};

class SubsetDomainFeature : public DomainFeature
{
public:
  SubsetDomainFeature(const std::string& domainFile) :
    DomainFeature(domainFile) {}

protected:
  virtual void add(const std::map<std::string,float>& domainCounts, float count,
                   const MaybeLog& maybeLog,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;
};

class SparseSubsetDomainFeature : public DomainFeature
{
public:
  SparseSubsetDomainFeature(const std::string& domainFile) :
    DomainFeature(domainFile) {}

protected:
  virtual void add(const std::map<std::string,float>& domainCounts, float count,
                   const MaybeLog& maybeLog,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;

};

class IndicatorDomainFeature : public DomainFeature
{
public:
  IndicatorDomainFeature(const std::string& domainFile) :
    DomainFeature(domainFile) {}

protected:
  virtual void add(const std::map<std::string,float>& domainCounts, float count,
                   const MaybeLog& maybeLog,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;
};


class SparseIndicatorDomainFeature : public DomainFeature
{
public:
  SparseIndicatorDomainFeature(const std::string& domainFile) :
    DomainFeature(domainFile) {}

protected:
  virtual void add(const std::map<std::string,float>& domainCounts, float count,
                   const MaybeLog& maybeLog,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;
};


class RatioDomainFeature : public DomainFeature
{
public:
  RatioDomainFeature(const std::string& domainFile) :
    DomainFeature(domainFile) {}

protected:
  virtual void add(const std::map<std::string,float>& domainCounts, float count,
                   const MaybeLog& maybeLog,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;
};


class SparseRatioDomainFeature : public DomainFeature
{
public:
  SparseRatioDomainFeature(const std::string& domainFile) :
    DomainFeature(domainFile) {}

protected:
  virtual void add(const std::map<std::string,float>& domainCounts, float count,
                   const MaybeLog& maybeLog,
                   std::vector<float>& denseValues,
                   std::map<std::string,float>& sparseValues) const;
};


}

#endif
