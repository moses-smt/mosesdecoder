/*
 *  Parameter.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

using namespace std;

#include "Parameter.h"

/** define allowed parameters */
Parameter::Parameter() 
{
	AddParam("NbestFile", "a", "file of nbest");
	AddParam("InputFeatureStatistics", "b", "file of input feature scores");
	AddParam("InputScoreStatistics", "c", "file of input score statistics");
	AddParam("OutputFeatureStatistics", "d", "file for output feature scores");
	AddParam("OutputScoreStatistics", "e", "file for output score statistics");
	AddParam("OutputBinaryMode", "f", "Binary mode for output files");
	AddParam("Reference", "g", "Reference");
    vector<string> stypes = ScorerFactory().getTypes();
    stringstream stypes_options;
    copy(stypes.begin(),stypes.end(),ostream_iterator<string>(stypes_options,"|"));
	AddParam("Score", "s", "Score type (" + stypes_options.str() + ")");
	AddParam("help", "h", "Print this help");
};

/** initialize a parameter, sub of constructor */
void Parameter::AddParam(const string &paramName, const string &description)
{
	m_description[paramName] = description;
}

/** initialize a parameter (including abbreviation), sub of constructor */
void Parameter::AddParam(const string &paramName, const string &abbrevName, const string &description)
{
	m_abbreviation[paramName] = abbrevName;
	m_description[paramName] = description;
}

/** print descriptions of all parameters */
void Parameter::Explain() {
	std::cerr << "Usage:" << std::endl;
	for(PARAM_STRING::const_iterator iterParam = m_description.begin(); iterParam != m_description.end(); iterParam++) 
		{
			const string paramName = iterParam->first;
			const string paramDescription = iterParam->second;
			std::cerr << "\t-" << paramName;
			PARAM_STRING::const_iterator iterAbbr = m_abbreviation.find( paramName );
			if ( iterAbbr != m_abbreviation.end() )
				std::cerr << " (" << iterAbbr->second << ")";			
			std::cerr << ": " << paramDescription << std::endl;
		}
}

/** load all parameters from the command line */
bool Parameter::LoadParam(int argc, char* argv[]) 
{
	for(PARAM_STRING::const_iterator iterParam = m_description.begin(); iterParam != m_description.end(); iterParam++) 
	{
		const string paramName = iterParam->first;
		WriteParam("-" + paramName, paramName, argc, argv);
	}

	// ... also shortcuts
	for(PARAM_STRING::const_iterator iterParam = m_abbreviation.begin(); iterParam != m_abbreviation.end(); iterParam++) 
	{
		const string paramName = iterParam->first;
		const string paramShortName = iterParam->second;
		WriteParam("-" + paramShortName, paramName, argc, argv);
	}

	// logging of parameters that were set in either config or switch
	int verbose = 1;
	if (m_setting.find("verbose") != m_setting.end() &&
	    m_setting["verbose"].size() > 0)
	{
	  verbose = Scan<int>(m_setting["verbose"][0]);
	}
	
	if (verbose >= 1) { // only if verbose
	  std::cerr << "Defined parameters:" << std::endl;
	  for(PARAM_MAP::const_iterator iterParam = m_setting.begin() ; iterParam != m_setting.end(); iterParam++) {
	    std::cerr << "\t" << iterParam->first << ": ";
	    for ( size_t i = 0; i < iterParam->second.size(); i++ )
	      std::cerr << iterParam->second[i] << " ";
	    std::cerr << std::endl;
	  }
	}
	
	return true;
}


/** set parameters with command line swiches
 * \param paramSwitch (potentially short) name of switch
 * \param paramName full name of parameter
 * \param argc number of arguments on command line
 * \param argv values of paramters on command line */
void Parameter::WriteParam(const string &paramSwitch, const string &paramName, int argc, char* argv[])
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
	m_setting[paramName]; // defines the parameter, important for boolean switches
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

/** check whether an item on the command line is a switch or a value 
* \param token token on the command line to checked **/

bool Parameter::isOption(const char* token) {
  if (! token) return false;
  std::string tokenString(token);
  size_t length = tokenString.size();
  if (length > 0 && tokenString.substr(0,1) != "-") return false;
  if (length > 1 && tokenString.substr(1,1).find_first_not_of("0123456789") == 0) return true;
  return false;
}


