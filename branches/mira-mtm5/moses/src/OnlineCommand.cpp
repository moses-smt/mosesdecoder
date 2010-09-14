// $Id: OnlineCommand.cpp 3428 2010-09-13 17:55:23Z nicolabertoldi $
// vim:tabstop=2

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

#include <stdexcept>

#include "StaticData.h"  // needed for debugging purpose only

#include "OnlineCommand.h"
#include "Util.h"

using namespace std;

#define COMMAND_KEYWORD "@CMD@"

namespace Moses
{

OnlineCommand::OnlineCommand()	
{
	VERBOSE(3,"OnlineCommand::OnlineCommand()" << std::endl);
  VERBOSE(3,"COMMAND_KEYWORD: " << COMMAND_KEYWORD << std::endl);
  command_type = '\0';
  command_value = '\0';

	accepted_commands.push_back("-weight-l"); // weight(s) for language models
	accepted_commands.push_back("-weight-t"); // weight(s) for translation model components
	accepted_commands.push_back("-weight-d"); // weight(s) for distortion (reordering components)
	accepted_commands.push_back("-weight-w"); // weight for word penalty
	accepted_commands.push_back("-weight-u"); // weight for unknown words penalty
	accepted_commands.push_back("-weight-g"); // weight(s) for global lexical model components
	accepted_commands.push_back("-verbose"); // weights for translation model components
}
	
bool OnlineCommand::Parse(std::string& line) 
{
	VERBOSE(3,"OnlineCommand::Parse(std::string& line)" << std::endl);

	int next_string_pos = 0;
	string firststring = GetFirstString(line, next_string_pos);
	bool flag = false;

	if (firststring.compare(COMMAND_KEYWORD) == 0){
		
		command_type = GetFirstString(line, next_string_pos);


		for (vector<string>::const_iterator iterParam = accepted_commands.begin(); iterParam!=accepted_commands.end(); ++iterParam) {
    	if (command_type.compare(*iterParam) == 0){   //requested command is found
				command_value = line.substr(next_string_pos);
				flag = true;
			}
		}
		if (!flag){
			VERBOSE(3,"OnlineCommand::Parse: This command |" << command_type << "| is unknown." << std::endl);
	  }
		return true;
	}else{
		return false;
	}
}

void OnlineCommand::Execute() const
{
	std::cerr << "void OnlineCommand::Execute() const" << std::endl;
	VERBOSE(3,"OnlineCommand::Execute() const" << std::endl);
	StaticData &staticData = (StaticData&) StaticData::Instance();

	VERBOSE(1,"Handling online command: " << COMMAND_KEYWORD << " " << command_type << " " << command_value << std::endl);
  // weights
  vector<float> actual_weights;
  vector<float> weights;
  PARAM_VEC values;

  bool flag = false;
  for(vector<std::string>::const_iterator iterParam = accepted_commands.begin(); iterParam != accepted_commands.end(); iterParam++) 
  {
  	std::string paramName = *iterParam;
	
  	if (command_type.compare(paramName) == 0){   //requested command is paramName

			Tokenize(values, command_value);

		//remove initial "-" character
			paramName.erase(0,1);

			staticData.GetParameter()->OverwriteParam(paramName, values);

			staticData.ReLoadParameter();
    // check on weights

    vector<float> weights = staticData.GetAllWeights();
    IFVERBOSE(2) {
        TRACE_ERR("The score component vector looks like this:\n" << staticData.GetScoreIndexManager());
        TRACE_ERR("The global weight vector looks like this:");
        for (size_t j=0; j<weights.size(); j++) { TRACE_ERR(" " << weights[j]); }
        TRACE_ERR("\n");
    }


			flag = true;
  	}
	}
	if (!flag){
		TRACE_ERR("ERROR: The command |" << command_type << "| is unknown." << std::endl);
	}
}

void OnlineCommand::Print(std::ostream& out) const
{
	VERBOSE(3,"OnlineCommand::Print(std::ostream& out) const" << std::endl);
	out << command_type << " -> " << command_value << "\n";
}
 
void OnlineCommand::Clean()
{
  VERBOSE(3,"OnlineCommand::Clean() const" << std::endl);
  command_type = '\0';
  command_value = '\0';
}


}

