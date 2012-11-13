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

#include <sstream>
#include <iostream>
#include "UserMessage.h"

using namespace std;

namespace Moses
{

const size_t MAX_MSG_QUEUE = 5;

bool UserMessage::m_toStderr	= true;
bool UserMessage::m_toQueue		= false;
queue<string> UserMessage::m_msgQueue;

void UserMessage::Add(const string &msg)
{
  if (m_toStderr) {
    cerr << "ERROR:" << msg << endl;
  }
  if (m_toQueue) {
    if (m_msgQueue.size() >= MAX_MSG_QUEUE)
      m_msgQueue.pop();
    m_msgQueue.push(msg);
  }
}

string UserMessage::GetQueue()
{
  stringstream strme("");
  while (!m_msgQueue.empty()) {
    strme << m_msgQueue.front() << endl;
    m_msgQueue.pop();
  }
  return strme.str();
}

}



