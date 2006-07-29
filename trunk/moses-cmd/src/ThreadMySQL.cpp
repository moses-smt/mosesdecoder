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

#include "Sentence.h"
#include "Hypothesis.h"
#include "ThreadMySQL.h"
#include "TypeDef.h"

using namespace std;

void ThreadMySQL::operator()()
{
	for (int i = 0; i < 100 ; i++)
	{
		TRACE_ERR("from class" << std::endl);

		for (int currSentence = 0 ; currSentence < 5 ; currSentence++)
		{
		}
	}
}

Sentence *ThreadMySQL::GetSentence()
{
	if (m_sentenceList.size())
	{
		Sentence *ret = m_sentenceList.front();
		m_sentenceList.erase(m_sentenceList.begin());
		return ret;
	}
	else
	{
		return NULL;
	}
}

// helper fn
void EscapeString(string &sql)
{
	if (!sql.size()) 
	{
		return;
	}

	for (unsigned int i = 0; i < sql.size(); i++) 
	{
		switch (sql[i])
		{
			case '\\':
				sql.insert(i, "\\", 1);
				i++;
				break;
			case '\"':
				sql.insert(i, "\\", 1);
				i++;
				break;
			case '\'':		
				sql.insert(i, "\\", 1);
				i++;
				break;
		}
	}
}

void ThreadMySQL::SetTranslation(const Hypothesis *hypo, long translationId)
{
	stringstream strme;

	// tell people we've finished this translation
	if (hypo == NULL)
	{
		strme << "UPDATE translation"
					<< " SET unparsed = ''"
					<< "		,completed_date = NOW()"
					<< " WHERE translation_id = " << translationId;
	}
	else
	{
		TRACE_ERR("BEST HYPO: " << *hypo << endl);
		// translated string
		strme << *hypo;
		string unparsed = strme.str();
		EscapeString(unparsed);

		// sql
		strme.str("");
		strme << "UPDATE translation"
			<< " SET unparsed = ' " << unparsed << "'"
			<< "		,completed_date = NOW()"
			<< "		,cost_translation = " << "0"
			<< "		,cost_LM = "					<< "0"
			<< "		,cost_distortion = "	<< "0"
			<< "		,cost_word_penalty = " << "0"
			<< "		,cost_future = "			<< "0"
			<< "		,cost_total = "				<< "0"
			<< " WHERE translation_id = " << translationId;
	}

	mysqlpp::Query query = m_conn.query();
	query.exec(strme.str());
	query.reset();

	
	// delete any previous results
	strme.str("");
	strme << "DELETE word_occurence WHERE translation_id = "
				<< translationId;
	query.exec(strme.str());
	query.reset();
				
	for (size_t pos = 0 ; pos < hypo->GetSize() ; pos++)
	{

		strme.str("");
		strme << "INSERT word_occurence (translation_id, ordering, surface, pos, stem, morphology)"
					<< " VALUES (" << translationId << "," << pos << ",";
			
		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *factor = hypo->GetFactor(pos, factorType);
			if (factor == NULL)
			{
				strme << "NULL,";
			}
			else
			{
				string str = factor->GetString();
				EscapeString(str);
				strme << "'" << str << "',";
			}
		}

		string sql = strme.str();
		sql = sql.substr(0, sql.size() - 1);
		sql += ")";

		query.exec(sql);
		query.reset();
	}
}

Sentence *ThreadMySQL::ReadSentence(mysqlpp::Connection &conn)
{
	stringstream strme;
	strme << "SELECT Tin.translation_id idIn, Tout.translation_id idOut"
				<< " FROM   translation Tin, translation Tout"
				<< " WHERE  Tin.datastream_id		= " << m_inputStreamId
				<< " AND	  Tout.datastream_id	= " << m_outputStreamId
				<< " AND		Tin.sentence_id			= Tout.sentence_id"
				<< " AND		Tout.completed_date IS NULL"
				<< " AND    (Tout.scheduled_date IS NULL"
				<< "				OR Tout.scheduled_date < DATE_SUB(NOW(), INTERVAL 20 MINUTE) )"
				<< " LIMIT  1"
				<< " FOR UPDATE;";

	mysqlpp::Query query = conn.query();
	query << strme.str();
	mysqlpp::Result res = query.store();
	query.reset();
	mysqlpp::Row row;
	if (res && (row = res.at(0)))
	{ // found a sentence to translate. get all words in it
//		cerr << row["translation_id"] << endl;
		long	idIn	= row["idIn"]
					,idOut= row["idOut"];

		Sentence *sentence = new Sentence(Input);
		sentence->SetTranslationId(idOut);

		strme.str("");
		strme	<< "SELECT surface, pos, stem, morphology"
					<< " FROM word_occurence"
					<< " WHERE translation_id = " << idIn
					<< " ORDER BY ordering;";

		query << strme.str();	
		res = query.store();
		if (res)
		{
			for (int i = 0; row = res.at(i); ++i)
			{
				FactorArray &factorArray = sentence->AddWord();
				if (!row["surface"].is_null())
					factorArray[Surface] = m_factorCollection.AddFactor(Input, Surface, row["surface"].c_str());
				if (!row["pos"].is_null())
					factorArray[POS] = m_factorCollection.AddFactor(Input, POS, row["pos"].c_str());
				if (!row["stem"].is_null())
					factorArray[Stem] = m_factorCollection.AddFactor(Input, Stem, row["stem"].c_str());
				if (!row["morphology"].is_null())
					factorArray[Morphology] =m_factorCollection.AddFactor(Input, Morphology, row["morphology"].c_str());
			}

			// update output translation to let people know we're currently translating it
			query.reset();
			strme.str("");
			strme	<< "UPDATE translation SET scheduled_date = NOW() WHERE translation_id = "
						<< idOut;
			query.exec(strme.str());
		}

		return sentence;
	}
	else
	{ // no more sentences
		return NULL;
	}
}

