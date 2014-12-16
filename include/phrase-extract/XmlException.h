/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

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
#ifndef XMLEXCEPTION_H_INCLUDED_
#define XMLEXCEPTION_H_INCLUDED_

#include <string>

namespace MosesTraining
{

class XmlException
{
public:
  XmlException(const std::string & msg)
    : m_msg(msg) {
  }

  const std::string &
  getMsg() const {
    return m_msg;
  }

private:
  std::string m_msg;
};

}

#endif
