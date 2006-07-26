// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (c) 2006 University of Edinburgh
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
			this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, 
			this list of conditions and the following disclaimer in the documentation 
			and/or other materials provided with the distribution.
    * Neither the name of the University of Edinburgh nor the names of its contributors 
			may be used to endorse or promote products derived from this software 
			without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

// example file on how to use moses library

#pragma once

#include <string>
#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include "mysql++.h"
#include "InputOutput.h"
#include "ThreadMySQL.h"

class LatticePathList;

class IOMySQL : public InputOutput
{
protected:
	std::string m_db, m_host, m_login, m_password;
	mysqlpp::Connection						m_conn;
	long													m_inputStreamId, m_outputStreamId;
	const std::vector<FactorType>	&m_factorOrder;
	const FactorTypeSet						&m_inputFactorUsed;
	FactorCollection							&m_factorCollection;
	ThreadMySQL										*m_threadMySQL;
	boost::thread									*m_inputThread;

public:
	IOMySQL(const std::string &host
				, const std::string &db
				, const std::string &login
				, const std::string &password
				, long inputStreamId
				, long outputStreamId
				, const std::vector<FactorType>	&factorOrder
				, const FactorTypeSet						&inputFactorUsed
				, FactorCollection							&factorCollection);
	~IOMySQL();
	void Connect(mysqlpp::Connection &conn);
	InputType *GetInput(InputType*);
	void SetOutput(const Hypothesis *hypo, long translationId, bool reportSourceSpan, bool reportAllFactors);
	void SetNBest(const LatticePathList &nBestList, long translationId)
	{
	}

};

