// $Id: Parameter.h 3253 2010-05-13 14:12:05Z chardmeier $

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
#include <map>
#include <vector>
#include "TypeDef.h"

namespace Moses
{

typedef std::vector<std::string>						PARAM_VEC;
typedef std::map<std::string, PARAM_VEC > 	PARAM_MAP;
typedef std::map<std::string, bool>         PARAM_BOOL;
typedef std::map<std::string, std::string > PARAM_STRING;

/** Handles parameter values set in config file or on command line.
 * Process raw parameter data (names and values as strings) for StaticData 
 * to parse; to get useful values, see StaticData. */
class Parameter
{
protected:
	PARAM_MAP m_setting;
	PARAM_BOOL m_valid;
	PARAM_STRING m_abbreviation;
	PARAM_STRING m_description;

	std::string FindParam(const std::string &paramSwitch, int argc, char* argv[]);
	void OverwriteParam(const std::string &paramSwitch, const std::string &paramName, int argc, char* argv[]);
	bool ReadConfigFile(const std::string &filePath );
	bool FilesExist(const std::string &paramName, int fieldNo, std::vector<std::string> const& fileExtension=std::vector<std::string>(1,""));
	bool isOption(const char* token);
	bool Validate();

	void AddParam(const std::string &paramName, const std::string &description);
	void AddParam(const std::string &paramName, const std::string &abbrevName, const std::string &description);

	void PrintCredit();

public:
	Parameter();
	~Parameter();
	bool LoadParam(int argc, char* argv[]);
	bool LoadParam(const std::string &filePath);
	void Explain();

	/** return a vector of strings holding the whitespace-delimited values on the ini-file line corresponding to the given parameter name */
	const PARAM_VEC &GetParam(const std::string &paramName)
	{
		return m_setting[paramName];
	}
	/** check if parameter is defined (either in moses.ini or as switch) */
	bool isParamSpecified(const std::string &paramName)
	{
	  return  m_setting.find( paramName ) != m_setting.end();
	}

};

}

#endif
