/// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_Parameter_h
#define moses_Parameter_h

#include <string>
#include <set>
#include <map>
#include <vector>
#include "TypeDef.h"
#include "Util.h"
#include <boost/program_options.hpp>

namespace Moses
{
typedef std::vector<std::string>            PARAM_VEC;
typedef std::map<std::string, PARAM_VEC >   PARAM_MAP;
typedef std::map<std::string, bool>         PARAM_BOOL;
typedef std::map<std::string, std::string > PARAM_STRING;

/** Handles parameter values set in config file or on command line.
 * Process raw parameter data (names and values as strings) for StaticData
 * to parse; to get useful values, see StaticData.
 */
class Parameter
{
  typedef boost::program_options::options_description options_description;
  typedef boost::program_options::value_semantic value_semantic;
protected:
  PARAM_MAP m_setting;
  PARAM_BOOL m_valid;
  PARAM_STRING m_abbreviation;
  PARAM_STRING m_description;
  PARAM_STRING m_fullname;
  // std::map<char,std::set<std::string> > m_confusable;
  // stores long parameter names that start with a letter that is also a short option.
  options_description m_options;

  std::map<std::string, std::vector<float> >  m_weights;

  std::string FindParam(const std::string &paramSwitch, int argc, char const* argv[]);
  void OverwriteParam(const std::string &paramSwitch, const std::string &paramName,
                      int argc, char const* argv[]);
  bool ReadConfigFile(const std::string &filePath );
  bool FilesExist(const std::string &paramName, int fieldNo, std::vector<std::string> const& fileExtension=std::vector<std::string>(1,""));
  bool isOption(const char* token);
  bool Validate();

  void
  AddParam(options_description& optgroup,
           value_semantic const* optvalue,
           std::string const& paramName,
           std::string const& description);

  void
  AddParam(options_description& optgroup,
           std::string const &paramName,
           std::string const &description);

  void
  AddParam(options_description& optgroup,
           value_semantic const* optvalue,
           std::string const& paramName,
           std::string const& abbrevName,
           std::string const& description);

  void
  AddParam(options_description& optgroup,
           std::string const& paramName,
           std::string const& abbrevName,
           std::string const& description);

  void PrintCredit();
  void PrintFF() const;

  void SetWeight(const std::string &name, size_t ind, float weight);
  void SetWeight(const std::string &name, size_t ind, const std::vector<float> &weights);
  void AddWeight(const std::string &name, size_t ind, const std::vector<float> &weights);
  void ConvertWeightArgs();
  void ConvertWeightArgsSingleWeight(const std::string &oldWeightName, const std::string &newWeightName);
  void ConvertWeightArgsPhraseModel(const std::string &oldWeightName);
  void ConvertWeightArgsLM();
  void ConvertWeightArgsDistortion();
  void ConvertWeightArgsGeneration(const std::string &oldWeightName, const std::string &newWeightName);
  void ConvertWeightArgsPhrasePenalty();
  void ConvertWeightArgsWordPenalty();
  void ConvertPhrasePenalty();
  void CreateWeightsMap();
  void CreateWeightsMap(const PARAM_VEC &vec);
  void WeightOverwrite();
  void AddFeature(const std::string &line);
  void AddFeaturesCmd();


public:
  Parameter();
  ~Parameter();
  bool LoadParam(int argc, char const* argv[]);
  bool LoadParam(const std::string &filePath);
  void Explain();

  /** return a vector of strings holding the whitespace-delimited values on the ini-file line corresponding to the given parameter name */
  const PARAM_VEC *GetParam(const std::string &paramName) const;

  /** check if parameter is defined (either in moses.ini or as switch) */
  bool isParamSpecified(const std::string &paramName) const {
    return  m_setting.find( paramName ) != m_setting.end();
  }

  void OverwriteParam(const std::string &paramName, PARAM_VEC values);

  std::vector<float> GetWeights(const std::string &name);
  const std::map<std::string, std::vector<float> > &GetAllWeights() const {
    return m_weights;
  }
  std::set<std::string> GetWeightNames() const;

  const PARAM_MAP &GetParams() const {
    return m_setting;
  }

  void Save(const std::string path);

  template<typename T>
  void SetParameter(T &var, const std::string &name, const T &defaultValue) const {
    const PARAM_VEC *params = GetParam(name);
    if (params && params->size()) {
      var = Scan<T>( params->at(0));
    } else {
      var = defaultValue;
    }
  }

  void SetParameter(bool& var, std::string const& name);

  bool SetBooleanSwitch(bool& val, std::string const name) {
    // issues a warning if format is wrong
    const PARAM_VEC *params = GetParam(name);
    val = (params && params->size());
    if (val && params->size() != 1) {
      TRACE_ERR("ERROR: wrong format for switch -" << name);
      return false;
    }
    return true;
  }

};

template<>
void Parameter::SetParameter<bool>(bool &var, const std::string &name, const bool &defaultValue) const;

}

#endif
