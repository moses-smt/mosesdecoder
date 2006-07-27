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

#include <sstream>
#include <string>
#include <iostream>
#include "Hypothesis.h"
#include "Sentence.h"
#include "FactorCollection.h"
#include "IOMySQL.h"
	// redefines max() ??

using namespace std;

// sleep helper fns
#ifdef WIN32
#include "windows.h"
// get Sleep(millisec)

#endif
#ifndef WIN32
void Sleep(unsigned int mSec)
{
    usleep( mSec * 1000 );
}
#endif

IOMySQL::IOMySQL(const string &host
								, const string &db
								, const string &login
								, const string &password
								, long inputStreamId
								, long outputStreamId
								, const std::vector<FactorType>	&factorOrder
								, const FactorTypeSet						&inputFactorUsed
								, FactorCollection							&factorCollection)
:m_db(db)
,m_host(host)
,m_login(login)
,m_password(password)
,m_inputStreamId(inputStreamId)
,m_outputStreamId(outputStreamId)
,m_factorOrder(factorOrder)
,m_inputFactorUsed(inputFactorUsed)
,m_factorCollection(factorCollection)
{
	Connect(m_conn);
	m_threadMySQL = new ThreadMySQL(m_conn, inputStreamId, outputStreamId, factorCollection);
	m_inputThread	= new boost::thread(*m_threadMySQL);
}

IOMySQL::~IOMySQL()
{
	//m_inputThread->join();
	delete m_inputThread;
	delete m_threadMySQL;
}

void IOMySQL::Connect(mysqlpp::Connection &conn)
{
	TRACE_ERR(m_db << " " << m_host << " " << m_login << " " << m_password << endl);
	while (!conn.connect(m_db.c_str(), m_host.c_str(), m_login.c_str(), m_password.c_str()))
	{ // try again in a few seconds
		TRACE_ERR("connection failed, trying again in 10 seconds" << endl);
		Sleep(10000);
	}
}

InputType *IOMySQL::GetInput(InputType*)
{
	TRACE_ERR("boo" << endl);
	//return m_threadMySQL->GetSentence();
	return NULL;
}

void IOMySQL::SetOutput(const Hypothesis *hypo, long translationId, bool reportSourceSpan, bool reportAllFactors)
{
	//m_threadMySQL->SetTranslation(hypo, translationId);
}

