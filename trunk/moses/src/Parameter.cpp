// $Id$

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

#include <iostream>
#include <fstream>
#include <sstream>
#include "boost/filesystem/operations.hpp" // includes boost/filesystem/path.hpp
#include "boost/filesystem/fstream.hpp"    // ditto
#include <boost/algorithm/string.hpp>
#include "Parameter.h"
#include "Util.h"
#include "InputFileStream.h"
#include "UserMessage.h"

using namespace std;

PARAM_VEC &Parameter::AddParam(const string &paramName)
{
	return m_setting[paramName];
}

// default parameter values
Parameter::Parameter() 
{
	AddParam("ttable-file");
	AddParam("lmodel-file");
	AddParam("ttable-limit");
	AddParam("weight-d");
	AddParam("weight-l");
	AddParam("weight-t");
	AddParam("weight-w");
	AddParam("weight-e"); //source word deletion overall weight
	AddParam("weight-generation");
	AddParam("weight-i");
	AddParam("mapping");
	AddParam("n-best-list");
	AddParam("beam-threshold");
	AddParam("distortion-limit");
	AddParam("input-factors");
	AddParam("mysql");
	AddParam("input-file");
	AddParam("cache-path");
	AddParam("input-file");
 	AddParam("distortion-file");
 	AddParam("distortion");
	AddParam("stack");
	AddParam("verbose");
	AddParam("report-source-span");
	AddParam("report-all-factors");
	AddParam("drop-unknown");
	AddParam("inputtype");
	AddParam("max-trans-opt-per-coverage");
}

// check if parameter settings make sense
bool Parameter::Validate() 
{
	bool ret = true;

  // required parameters
	if (m_setting["ttable-file"].size() == 0)
	{
		UserMessage::Add("No phrase translation table (ttable-file)");
		ret = false;
	}

	if (m_setting["lmodel-file"].size() == 0)
	{
		UserMessage::Add("No language model (lmodel-file)");
		ret = false;
	}

	if (m_setting["lmodel-file"].size() != m_setting["weight-l"].size()) 
	{
		stringstream errorMsg("");
		errorMsg <<  static_cast<int>(m_setting["lmodel-file"].size()) 
						<< " language model files given (lmodel-file), but " 
						<< static_cast<int>(m_setting["weight-l"].size())
						<< " weights (weight-l)";
		UserMessage::Add(errorMsg.str());
		ret = false;
	}

  // do files exist?
	// phrase tables
	if (ret) 
		{
			std::vector<std::string> ext;
			// standard phrase table extension (i.e. full name has to be specified)
			ext.push_back("");
			// alternative file extension for binary phrase table format:
			ext.push_back(".binphr.idx");
			ret = FilesExist("ttable-file", 3,ext);
		}
	// generation tables
	if (ret)
		ret = FilesExist("generation-file", 2);
	// language model
	if (ret)
		ret = FilesExist("lmodel-file", 3);

	return ret;
}

bool Parameter::FilesExist(const string &paramName, size_t tokenizeIndex,std::vector<std::string> const& extensions)
{
	using namespace boost::filesystem;
	
	typedef std::vector<std::string> StringVec;
	StringVec::const_iterator iter;

	PARAM_MAP::const_iterator iterParam = m_setting.find(paramName);
	if (iterParam == m_setting.end())
	{ // no param. therefore nothing to check
		return true;
	}
	const StringVec &pathVec = (*iterParam).second;
	for (iter = pathVec.begin() ; iter != pathVec.end() ; ++iter)
	{
		StringVec vec = Tokenize(*iter);
		if (tokenizeIndex >= vec.size())
		{
			stringstream errorMsg("");
			errorMsg << "Expected " << tokenizeIndex << " tokens per"
							<< " entry in '" << paramName << "', but only found "
							<< vec.size();
			UserMessage::Add(errorMsg.str());
			return false;
		}
		const string &pathStr = vec[tokenizeIndex];

		bool fileFound=0;
		for(size_t i=0;i<extensions.size() && !fileFound;++i)
			{
				path filePath(pathStr+extensions[i], native);
				fileFound|=exists(filePath);
			}
		if(!fileFound)
			{
				stringstream errorMsg("");
				errorMsg << "File " << pathStr << " does not exists";
				UserMessage::Add(errorMsg.str());
				return false;
			}
			
	}
	return true;
}

// TODO arg parsing like this does not belong in the library, it belongs
// in moses-cmd
string Parameter::FindParam(const string &paramSwitch, int argc, char* argv[])
{
	for (int i = 0 ; i < argc ; i++)
	{
		if (string(argv[i]) == paramSwitch)
		{
			if (i+1 < argc)
			{
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

bool isOption(const char* x) {
  if (!x) return false;
  std::string s(x);
  size_t len = s.size();
  if (len > 0 && s.substr(0,1) != "-") return false;
  if (len > 1 && s.substr(1,1).find_first_not_of("0123456789") == 0) return true;
  return false;
}

void Parameter::OverwriteParam(const string &paramSwitch, const string &paramName, int argc, char* argv[])
{
	int startPos = -1;
	for (int i = 0 ; i < argc ; i++)
	{
		if (string(argv[i]) == paramSwitch)
		{
			startPos = i+1;
			break;
		}
	}
	if (startPos < 0)
		return;

	int index = 0;
	while (startPos < argc && (!isOption(argv[startPos])))
	{
		if (m_setting[paramName].size() > (size_t)index)
			m_setting[paramName][index] = argv[startPos];
		else
			m_setting[paramName].push_back(argv[startPos]);
		index++;
		startPos++;
	}
}

// TODO this should be renamed to have at least a plural name
bool Parameter::LoadParam(int argc, char* argv[]) 
{
	// config file (-f) arg mandatory
	string configPath;
	if ( (configPath = FindParam("-f", argc, argv)) == "" 
		&& (configPath = FindParam("-config", argc, argv)) == "")
	{
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

	string inFile = FindParam("-i", argc, argv);
	if (inFile != "")
		m_setting["input-file"].push_back(inFile);

	// overwrite weights
	OverwriteParam("-dl", "distortion-limit", argc, argv);
	OverwriteParam("-d", "weight-d", argc, argv);
	OverwriteParam("-lm", "weight-l", argc, argv);
	OverwriteParam("-tm", "weight-t", argc, argv);
	OverwriteParam("-w", "weight-w", argc, argv);
	OverwriteParam("-e", "weight-e", argc, argv);
	OverwriteParam("-g", "weight-generation", argc, argv);
	OverwriteParam("-n-best-list", "n-best-list", argc, argv);
	OverwriteParam("-s", "stack", argc, argv);
	OverwriteParam("-v", "verbose", argc, argv);
	OverwriteParam("-report-source-span", "report-source-span", argc, argv);
	OverwriteParam("-report-all-factors", "report-all-factors", argc, argv);
	OverwriteParam("-drop-unknown", "drop-unknown", argc, argv);
	OverwriteParam("-inputtype","inputtype",argc,argv);
	OverwriteParam("-I","weight-i",argc,argv);
	OverwriteParam("-ttable-limit","ttable-limit",argc,argv);
	OverwriteParam("-lmodel-file","lmodel-file",argc,argv);
	OverwriteParam("-beam-threshold","beam-threshold",argc,argv);
	//	OverwriteParam("-","",argc,argv);


  // check if parameters make sense
	return Validate();
}

// read parameters from a configuration file
bool Parameter::ReadConfigFile( string filePath ) 
{
	InputFileStream inFile(filePath);
	string line, paramName;
	while(getline(inFile, line)) 
	{
		// comments
		size_t comPos = line.find_first_of("#");
		if (comPos != string::npos)
			line = line.substr(0, comPos);
		// trim leading and trailing spaces/tabs
		boost::trim(line);

		if (line[0]=='[') 
		{ // new parameter
			for (size_t currPos = 0 ; currPos < line.size() ; currPos++)
			{
				if (line[currPos] == ']')
				{
					paramName = line.substr(1, currPos - 1);
					break;
				}
			}
		}
    else if (line != "") 
		{ // add value to parameter
			m_setting[paramName].push_back(line);
		}
	}
	return true;
}
