// $Id: OnlineCommand.h 3428 2010-09-13 17:55:23Z nicolabertoldi $

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

#ifndef moses_OnlineCommand_h
#define moses_OnlineCommand_h

#include <string>
#include "InputType.h"
#include "StaticData.h"

namespace Moses
{

/***
 * A class used specifically to read online commnds to modify on the fly system' parameters
 */
class OnlineCommand 
{

 private:

	std::string command_type;
	std::string command_value;
	PARAM_VEC accepted_commands;

 public:
	OnlineCommand();

	bool Parse(std::string& str);
	void Print(std::ostream& out = std::cerr) const;
	void Execute() const;
	void Clean();

	inline std::string GetType(){ return command_type; };
	inline std::string GetValue(){ return command_value; };
};


}

#endif
