/*
 *  Parameter.h
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 9/7/06.
 *  Copyright 2006 ITC-irst. All rights reserved.
 *
 */

#ifndef PARAMETER_H
#define PARAMETER_H

using namespace std;

#include <vector>
#include <map>
#include <iostream>
#include <string>

#include "Util.h"

typedef std::vector<std::string>            PARAM_VEC;
typedef std::map<std::string, PARAM_VEC >   PARAM_MAP;
typedef std::map<std::string, std::string > PARAM_STRING;


/** Handles parameter values set in config file or on command line.
 * Process raw parameter data (names and values as strings)  */
class Parameter
{
protected:
	PARAM_MAP m_setting;
	PARAM_STRING m_abbreviation;
	PARAM_STRING m_description;
	
	void AddParam(const std::string &paramName, const std::string &description);
	void AddParam(const std::string &paramName, const std::string &abbrevName, const std::string &description);
	void WriteParam(const std::string &paramSwitch, const std::string &paramName, int argc, char* argv[]);
	bool isOption(const char* token);
	
public:
		Parameter();
	bool LoadParam(int argc, char* argv[]);
	bool isSet(string token) {return m_setting.find(token) != m_setting.end();}
	void Explain();
	
	/** return a vector of strings holding the whitespace-delimited values on the ini-file line corresponding to the given parameter name */
	const PARAM_VEC &GetParam(const std::string &paramName)	{		return m_setting[paramName];	}
};

#endif

