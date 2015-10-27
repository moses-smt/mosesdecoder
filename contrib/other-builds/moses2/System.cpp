/*
 * System.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <string>
#include <iostream>
#include <boost/foreach.hpp>
#include "System.h"
#include "FeatureFunction.h"
#include "moses/Util.h"
#include "util/exception.hh"

using namespace std;

System::System(const Moses::Parameter &params)
:m_featureFunctions(*this)
,m_params(params)
{
	m_featureFunctions.LoadFeatureFunctions();
	LoadWeights();
}

System::~System() {
}

void System::LoadWeights()
{
  const Moses::PARAM_VEC *weightParams = m_params.GetParam("weight");
  UTIL_THROW_IF2(weightParams == NULL, "Must have [weight] section");

  BOOST_FOREACH(const std::string &line, *weightParams) {
	  m_weights.CreateFromString(m_featureFunctions, line);
  }
}



