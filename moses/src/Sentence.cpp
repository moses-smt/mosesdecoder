// $Id$
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

#include "Sentence.h"
#include "PhraseDictionaryMemory.h"
#include "TranslationOptionCollectionText.h"
#include "StaticData.h"
#include "Util.h"

namespace Moses
{
int Sentence::Read(std::istream& in,const std::vector<FactorType>& factorOrder) 
{
	const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
	std::string line;
	std::map<std::string, std::string> meta;

	if (getline(in, line, '\n').eof())	
			return 0;
	line = Trim(line);
  meta = ProcessAndStripSGML(line);

	if (meta.find("id") != meta.end()) { this->SetTranslationId(atol(meta["id"].c_str())); }
	
	//parse XML markup in translation line
	const StaticData &staticData = StaticData::Instance();
	if (staticData.GetXmlInputType() != XmlPassThrough)
		m_xmlOptionsList = ProcessAndStripXMLTags(line, *this);
	Phrase::CreateFromString(factorOrder, line, factorDelimiter);
	
	//only fill the vector if we are parsing XML
	if (staticData.GetXmlInputType() != XmlPassThrough ) {
		for (size_t i=0; i<GetSize();i++) {
			m_xmlCoverageMap.push_back(false);
		}
		for (std::vector<TranslationOption*>::const_iterator iterXMLOpts = m_xmlOptionsList.begin();
		        iterXMLOpts != m_xmlOptionsList.end(); iterXMLOpts++) {
			//m_xmlOptionsList will be empty for XmlIgnore
			for(size_t j=(**iterXMLOpts).GetSourceWordsRange().GetStartPos();j<=(**iterXMLOpts).GetSourceWordsRange().GetEndPos();j++) {
				m_xmlCoverageMap[j]=true;
				
			}
		}
	}
		
	return 1;
}

TranslationOptionCollection* 
Sentence::CreateTranslationOptionCollection() const 
{
	size_t maxNoTransOptPerCoverage = StaticData::Instance().GetMaxNoTransOptPerCoverage();
	float transOptThreshold = StaticData::Instance().GetTranslationOptionThreshold();
	TranslationOptionCollection *rv= new TranslationOptionCollectionText(*this, maxNoTransOptPerCoverage, transOptThreshold);
	assert(rv);
	return rv;
}
void Sentence::Print(std::ostream& out) const
{
	out<<*static_cast<Phrase const*>(this)<<"\n";
}


bool Sentence::XmlOverlap(size_t startPos, size_t endPos) const {
	for (size_t pos = startPos; pos <=  endPos ; pos++)
		{
			if (pos < m_xmlCoverageMap.size() && m_xmlCoverageMap[pos]) {
				return true;
				}
		}
		return false;
}

void Sentence::GetXmlTranslationOptions(std::vector <TranslationOption*> &list, size_t startPos, size_t endPos) const {
	//iterate over XmlOptions list, find exact source/target matches
	const std::vector<FactorType> &outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
	
	for (std::vector<TranslationOption*>::const_iterator iterXMLOpts = m_xmlOptionsList.begin();
	        iterXMLOpts != m_xmlOptionsList.end(); iterXMLOpts++) {
		if (startPos == (**iterXMLOpts).GetSourceWordsRange().GetStartPos() && endPos == (**iterXMLOpts).GetSourceWordsRange().GetEndPos()) {
 			list.push_back(*iterXMLOpts);
		}
	}
}


std::string Sentence::ParseXmlTagAttribute(const std::string& tag,const std::string& attributeName){
	/*TODO deal with unescaping \"*/
	string tagOpen = attributeName + "=\"";
	size_t contentsStart = tag.find(tagOpen);
	if (contentsStart == std::string::npos) return "";
	contentsStart += tagOpen.size();
	size_t contentsEnd = tag.find_first_of('"',contentsStart+1);
	if (contentsEnd == std::string::npos) {
		TRACE_ERR("Malformed XML attribute: "<< tag);
		return "";	
	}
	size_t possibleEnd;
	while (tag.at(contentsEnd-1) == '\\' && (possibleEnd = tag.find_first_of('"',contentsEnd+1)) != std::string::npos) {
		contentsEnd = possibleEnd;
	}
	return tag.substr(contentsStart,contentsEnd-contentsStart);
}
 
}

