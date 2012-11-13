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

#ifndef moses_UserMessage_h
#define moses_UserMessage_h

#include <string>
#include <queue>

namespace Moses
{

/** User warnings/error messages.
 * Not the same as tracing messages, this should be usable even if Moses front-end if GUI
 */
class UserMessage
{
protected:
  static bool m_toStderr, m_toQueue;
  static std::queue<std::string> m_msgQueue;

public:
  //! whether messages to go to stderr, a queue to later display, or both
  static void SetOutput(bool toStderr, bool toQueue) {
    m_toStderr	= toStderr;
    m_toQueue		= toQueue;
  }
  //! add a message to be displayed
  static void Add(const std::string &msg);
  //! get all messages in queue. Each is on a separate line. Clear queue afterwards
  static std::string GetQueue();
};

}

#endif
