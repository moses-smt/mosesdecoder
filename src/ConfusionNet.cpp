// $Id$

#ifndef WIN32

#include "ConfusionNet.h"
#include <sstream>

#include "FactorCollection.h"
#include "Util.h"
#include "PhraseDictionaryTreeAdaptor.h"
#include "TranslationOptionCollectionConfusionNet.h"

ConfusionNet::ConfusionNet(FactorCollection* p) 
	: InputType(),m_factorCollection(p) {}

void ConfusionNet::SetFactorCollection(FactorCollection *p) 
{
	m_factorCollection=p;
}
bool ConfusionNet::ReadF(std::istream& in,
												 const std::vector<FactorType>& factorOrder,
												 int format) 
{
	TRACE_ERR("read confusion net with format "<<format<<"\n");
	switch(format) 
		{
		case 0: return ReadFormat0(in,factorOrder);
		case 1: return ReadFormat1(in,factorOrder);
		default: 
			std::cerr<<"ERROR: unknown format '"<<format
							 <<"' in ConfusionNet::Read\n";
		}
	return 0;
}

int ConfusionNet::Read(std::istream& in,
											 const std::vector<FactorType>& factorOrder, 
											 FactorCollection &factorCollection) 
{
	SetFactorCollection(&factorCollection);
	return ReadF(in,factorOrder,0);
}


void ConfusionNet::String2Word(const std::string& s,Word& w,
															 const std::vector<FactorType>& factorOrder) 
{
	std::vector<std::string> factorStrVector = Tokenize(s, "|");
	for(size_t i=0;i<factorOrder.size();++i)
		w.SetFactor(factorOrder[i],
								m_factorCollection->AddFactor(Input,factorOrder[i],
																							factorStrVector[i]));
}

bool ConfusionNet::ReadFormat0(std::istream& in,
															 const std::vector<FactorType>& factorOrder) 
{
	assert(m_factorCollection);
	Clear();
	std::string line;
	while(getline(in,line)) {
		std::istringstream is(line);
		std::string word;double prob;
		Column col;
		while(is>>word>>prob) {
			Word w;
			String2Word(word,w,factorOrder);
			if(prob<0.0) 
				{
					std::cerr<<"WARN: negative prob: "<<prob<<" ->set to 0.0\n";
					prob=0.0;
				}
			else if (prob>1.0)
				{
					std::cerr<<"WARN: prob > 1.0 : "<<prob<<" -> set to 1.0\n";
					prob=1.0;
				}
			col.push_back(std::make_pair(w,std::max(static_cast<float>(log(prob)),
																							LOWEST_SCORE)));
		}
		if(col.size()) {
			data.push_back(col);
			ShrinkToFit(data.back());
		}
		else break;
	}
	return !data.empty();
}
bool ConfusionNet::ReadFormat1(std::istream& in,
															 const std::vector<FactorType>& factorOrder) 
{
	assert(m_factorCollection);
	Clear();
	std::string line;
	if(!getline(in,line)) return 0;
	size_t s;
	if(getline(in,line)) s=atoi(line.c_str()); else return 0;
	data.resize(s);
	for(size_t i=0;i<data.size();++i) {
		if(!getline(in,line)) return 0;
		std::istringstream is(line);
		if(!(is>>s)) return 0;
		std::string word;double prob;
		data[i].resize(s);
		for(size_t j=0;j<s;++j)
			if(is>>word>>prob) {
				data[i][j].second=log(prob); 
				if(data[i][j].second<0) {
					std::cerr<<"WARN: neg costs: "<<data[i][j].second<<" -> set to 0\n";
					data[i][j].second=0.0;}
				String2Word(word,data[i][j].first,factorOrder);
			} else return 0;
	}
	return !data.empty();
}

void ConfusionNet::Print(std::ostream& out) const {
	out<<"conf net: "<<data.size()<<"\n";
	for(size_t i=0;i<data.size();++i) {
		out<<i<<" -- ";
		for(size_t j=0;j<data[i].size();++j)
			out<<"("<<data[i][j].first.ToString()<<", "<<data[i][j].second<<") ";
		out<<"\n";
	}
	out<<"\n\n";
}

Phrase ConfusionNet::GetSubString(const WordsRange&) const {
	std::cerr<<"ERROR: call to ConfusionNet::GetSubString\n";
	abort();
	return Phrase();}
const FactorArray& ConfusionNet::GetFactorArray(size_t) const {
	std::cerr<<"ERROR: call to ConfusionNet::GetFactorArray\n";
	abort();
}

std::ostream& operator<<(std::ostream& out,const ConfusionNet& cn) 
{
	cn.Print(out);return out;
}

TargetPhraseCollection const* ConfusionNet::
CreateTargetPhraseCollection(PhraseDictionaryBase const& d,
														 const WordsRange& r) const 
{
	if(PhraseDictionaryTreeAdaptor const* pdict=
		 dynamic_cast<PhraseDictionaryTreeAdaptor const*>(&d))
		return pdict->GetTargetPhraseCollection(*this,r);

	std::cerr<<"ERROR: wrong phrase dictionary type for confusion net decoding!"
		"  Has to be PhraseDictionaryTreeAdaptor\n";
	abort();
}

TranslationOptionCollection* 
ConfusionNet::CreateTranslationOptionCollection() const 
{
	return new TranslationOptionCollectionConfusionNet(*this);
}

#endif