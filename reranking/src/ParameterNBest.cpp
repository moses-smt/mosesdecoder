// $Id: $

/***********************************************************************
nbest - tool to process Moses n-best list
Copyright (C) 2008 Holger Schwenk, University of Le Mans, France

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

#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "ParameterNBest.h"
#include "Tools.h"

#include "Util.h" // from Moses
#include "InputFileStream.h"
#include "UserMessage.h"

using namespace std;

/** define allowed parameters */
ParameterNBest::ParameterNBest()
{
  AddParam("input-file", "i", "file name of the input n-best list");
  AddParam("output-file", "o", "file name of the output n-best list");
  AddParam("recalc", "r", "recalc global scores");
  AddParam("weights", "w", "coefficients of the feature functions");
  AddParam("sort", "s", "sort n-best list according to the global scores");
  AddParam("lexical", "l", "report number of lexically different hypothesis");
}

ParameterNBest::~ParameterNBest()
{
}

/** initialize a parameter, sub of constructor */
void ParameterNBest::AddParam(const string &paramName, const string &description)
{
  m_valid[paramName] = true;
  m_description[paramName] = description;
}

/** initialize a parameter (including abbreviation), sub of constructor */
void ParameterNBest::AddParam(const string &paramName, const string &abbrevName, const string &description)
{
  m_valid[paramName] = true;
  m_valid[abbrevName] = true;
  m_abbreviation[paramName] = abbrevName;
  m_description[paramName] = description;
}

/** print descriptions of all parameters */
void ParameterNBest::Explain()
{
  cerr << "Usage:" << endl;
  for(PARAM_STRING::const_iterator iterParam = m_description.begin(); iterParam != m_description.end(); iterParam++) {
    const string paramName = iterParam->first;
    const string paramDescription = iterParam->second;
    cerr <<  "\t-" << paramName;
    PARAM_STRING::const_iterator iterAbbr = m_abbreviation.find( paramName );
    if ( iterAbbr != m_abbreviation.end() )
      cerr <<  " (" << iterAbbr->second << ")";
    cerr <<  ": " << paramDescription << endl;
  }
}

/** check whether an item on the command line is a switch or a value
 * \param token token on the command line to checked **/

bool ParameterNBest::isOption(const char* token)
{
  if (! token) return false;
  std::string tokenString(token);
  size_t length = tokenString.size();
  if (length > 0 && tokenString.substr(0,1) != "-") return false;
  if (length > 1 && tokenString.substr(1,1).find_first_not_of("0123456789") == 0) return true;
  return false;
}

/** load all parameters from the configuration file and the command line switches */
bool ParameterNBest::LoadParam(const string &filePath)
{
  const char *argv[] = {"executable", "-f", filePath.c_str() };
  return LoadParam(3, (char**) argv);
}

/** load all parameters from the configuration file and the command line switches */
bool ParameterNBest::LoadParam(int argc, char* argv[])
{
  // config file (-f) arg mandatory
  string configPath;
  /*
  	if ( (configPath = FindParam("-f", argc, argv)) == ""
  		&& (configPath = FindParam("-config", argc, argv)) == "")
  	{
  		PrintCredit();

  		UserMessage::Add("No configuration file was specified.  Use -config or -f");
  		return false;
  	}
  	else
  	{
  		if (!ReadConfigFile(configPath))
  		{
  			UserMessage::Add("Could not read "+configPath);
  			return false;
  		}
  	}
  */

  // overwrite parameters with values from switches
  for(PARAM_STRING::const_iterator iterParam = m_description.begin(); iterParam != m_description.end(); iterParam++) {
    const string paramName = iterParam->first;
    OverwriteParam("-" + paramName, paramName, argc, argv);
  }

  // ... also shortcuts
  for(PARAM_STRING::const_iterator iterParam = m_abbreviation.begin(); iterParam != m_abbreviation.end(); iterParam++) {
    const string paramName = iterParam->first;
    const string paramShortName = iterParam->second;
    OverwriteParam("-" + paramShortName, paramName, argc, argv);
  }

  // logging of parameters that were set in either config or switch
  int verbose = 1;
  if (m_setting.find("verbose") != m_setting.end() &&
      m_setting["verbose"].size() > 0)
    verbose = Scan<int>(m_setting["verbose"][0]);
  if (verbose >= 1) { // only if verbose
    TRACE_ERR( "Defined parameters (per moses.ini or switch):" << endl);
    for(PARAM_MAP::const_iterator iterParam = m_setting.begin() ; iterParam != m_setting.end(); iterParam++) {
      TRACE_ERR( "\t" << iterParam->first << ": ");
      for ( size_t i = 0; i < iterParam->second.size(); i++ )
        TRACE_ERR( iterParam->second[i] << " ");
      TRACE_ERR( endl);
    }
  }

  // check for illegal parameters
  bool noErrorFlag = true;
  for (int i = 0 ; i < argc ; i++) {
    if (isOption(argv[i])) {
      string paramSwitch = (string) argv[i];
      string paramName = paramSwitch.substr(1);
      if (m_valid.find(paramName) == m_valid.end()) {
        UserMessage::Add("illegal switch: " + paramSwitch);
        noErrorFlag = false;
      }
    }
  }

  // check if parameters make sense
  return Validate() && noErrorFlag;
}

/** check that parameter settings make sense */
bool ParameterNBest::Validate()
{
  bool noErrorFlag = true;

  // required parameters
  if (m_setting["input-file"].size() == 0) {
    UserMessage::Add("No input-file");
    noErrorFlag = false;
  }

  if (m_setting["output-file"].size() == 0) {
    UserMessage::Add("No output-file");
    noErrorFlag = false;
  }

  if (m_setting["recalc"].size() > 0 && m_setting["weights"].size()==0) {
    UserMessage::Add("you need to spezify weight when recalculating global scores");
    noErrorFlag = false;
  }


  return noErrorFlag;
}

/** check whether a file exists */
bool ParameterNBest::FilesExist(const string &paramName, size_t tokenizeIndex,std::vector<std::string> const& extensions)
{
  typedef std::vector<std::string> StringVec;
  StringVec::const_iterator iter;

  PARAM_MAP::const_iterator iterParam = m_setting.find(paramName);
  if (iterParam == m_setting.end()) {
    // no param. therefore nothing to check
    return true;
  }
  const StringVec &pathVec = (*iterParam).second;
  for (iter = pathVec.begin() ; iter != pathVec.end() ; ++iter) {
    StringVec vec = Tokenize(*iter);
    if (tokenizeIndex >= vec.size()) {
      stringstream errorMsg("");
      errorMsg << "Expected at least " << (tokenizeIndex+1) << " tokens per emtry in '"
               << paramName << "', but only found "
               << vec.size();
      UserMessage::Add(errorMsg.str());
      return false;
    }
    const string &pathStr = vec[tokenizeIndex];

    bool fileFound=0;
    for(size_t i=0; i<extensions.size() && !fileFound; ++i) {
      fileFound|=FileExists(pathStr + extensions[i]);
    }
    if(!fileFound) {
      stringstream errorMsg("");
      errorMsg << "File " << pathStr << " does not exist";
      UserMessage::Add(errorMsg.str());
      return false;
    }
  }
  return true;
}

/** look for a switch in arg, update parameter */
// TODO arg parsing like this does not belong in the library, it belongs
// in moses-cmd
string ParameterNBest::FindParam(const string &paramSwitch, int argc, char* argv[])
{
  for (int i = 0 ; i < argc ; i++) {
    if (string(argv[i]) == paramSwitch) {
      if (i+1 < argc) {
        return argv[i+1];
      } else {
        stringstream errorMsg("");
        errorMsg << "Option " << paramSwitch << " requires a parameter!";
        UserMessage::Add(errorMsg.str());
        // TODO return some sort of error, not the empty string
      }
    }
  }
  return "";
}

/** update parameter settings with command line switches
 * \param paramSwitch (potentially short) name of switch
 * \param paramName full name of parameter
 * \param argc number of arguments on command line
 * \param argv values of paramters on command line */
void ParameterNBest::OverwriteParam(const string &paramSwitch, const string &paramName, int argc, char* argv[])
{
  int startPos = -1;
  for (int i = 0 ; i < argc ; i++) {
    if (string(argv[i]) == paramSwitch) {
      startPos = i+1;
      break;
    }
  }
  if (startPos < 0)
    return;

  int index = 0;
  m_setting[paramName]; // defines the parameter, important for boolean switches
  while (startPos < argc && (!isOption(argv[startPos]))) {
    if (m_setting[paramName].size() > (size_t)index)
      m_setting[paramName][index] = argv[startPos];
    else
      m_setting[paramName].push_back(argv[startPos]);
    index++;
    startPos++;
  }
}


/** read parameters from a configuration file */
bool ParameterNBest::ReadConfigFile( string filePath )
{
  InputFileStream inFile(filePath);
  string line, paramName;
  while(getline(inFile, line)) {
    // comments
    size_t comPos = line.find_first_of("#");
    if (comPos != string::npos)
      line = line.substr(0, comPos);
    // trim leading and trailing spaces/tabs
    line = Trim(line);

    if (line[0]=='[') {
      // new parameter
      for (size_t currPos = 0 ; currPos < line.size() ; currPos++) {
        if (line[currPos] == ']') {
          paramName = line.substr(1, currPos - 1);
          break;
        }
      }
    } else if (line != "") {
      // add value to parameter
      m_setting[paramName].push_back(line);
    }
  }
  return true;
}


void ParameterNBest::PrintCredit()
{
  cerr <<  "NBest - A tool to process Moses n-best lists" << endl
       << "Copyright (C) 2008 Holger Schwenk" << endl << endl

       << "This library is free software; you can redistribute it and/or" << endl
       << "modify it under the terms of the GNU Lesser General Public" << endl
       << "License as published by the Free Software Foundation; either" << endl
       << "version 2.1 of the License, or (at your option) any later version." << endl << endl

       << "This library is distributed in the hope that it will be useful," << endl
       << "but WITHOUT ANY WARRANTY; without even the implied warranty of" << endl
       << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU" << endl
       << "Lesser General Public License for more details." << endl << endl

       << "You should have received a copy of the GNU Lesser General Public" << endl
       << "License along with this library; if not, write to the Free Software" << endl
       << "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA" << endl << endl
       << "***********************************************************************" << endl << endl
       << "Built on " << __DATE__ << endl << endl

       << "Written by Holger Schwenk, Holger.Schwenk@lium.univ-lemans.fr" <<  endl << endl;
}

