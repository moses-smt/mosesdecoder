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

#pragma once

#include <string>
#include <map>
#include <vector>
#include "TypeDef.h"

typedef std::vector<std::string>						PARAM_VEC;
typedef std::map<std::string, PARAM_VEC > 	PARAM_MAP;

/***
 * store raw parameter data (names and values as strings) for StaticData to parse;
 * to get useful values, see StaticData
 */
class Parameter
{
protected:
	PARAM_MAP m_setting;

	std::string FindParam(const std::string &paramSwitch, int argc, char* argv[]);
	void OverwriteParam(const std::string &paramSwitch, const std::string &paramName, int argc, char* argv[]);
	bool ReadConfigFile( std::string filePath );
	bool FilesExist(const std::string &paramName, size_t tokenizeIndex,std::vector<std::string> const& fileExtension=std::vector<std::string>(1,""));

	bool Validate();

	PARAM_VEC &AddParam(const std::string &paramName);
	
public:
	Parameter();
	bool LoadParam(int argc, char* argv[]);

	/***
	 * return a vector of strings holding the whitespace-delimited values on the ini-file line 
	 * corresponding to the given parameter name
	 */
	const PARAM_VEC &GetParam(const std::string &paramName)
	{
		return m_setting[paramName];
	}

};

