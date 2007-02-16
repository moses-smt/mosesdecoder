// $Id$

#ifndef CONFUSIONNET_H_
#define CONFUSIONNET_H_
#include <vector>
#include <iostream>
#include "Word.h"
#include "InputType.h"

class FactorCollection;
class TranslationOptionCollection;
class Sentence;

class ConfusionNet : public InputType {
 public: 
	typedef std::vector<std::pair<Word,float> > Column;

 private:
	std::vector<Column> data;

	bool ReadFormat0(std::istream&,const std::vector<FactorType>& factorOrder);
	bool ReadFormat1(std::istream&,const std::vector<FactorType>& factorOrder);
	void String2Word(const std::string& s,Word& w,const std::vector<FactorType>& factorOrder);

 public:
	ConfusionNet();
	~ConfusionNet();

	ConfusionNet(Sentence const& s);
	
	const Column& GetColumn(size_t i) const {assert(i<data.size());return data[i];}
	const Column& operator[](size_t i) const {return GetColumn(i);}

	bool Empty() const {return data.empty();}
	size_t GetSize() const {return data.size();}
	void Clear() {data.clear();}

	bool ReadF(std::istream&,const std::vector<FactorType>& factorOrder,int format=0);
	void Print(std::ostream&) const;

	int Read(std::istream& in,const std::vector<FactorType>& factorOrder);
	
	Phrase GetSubString(const WordsRange&) const; //TODO not defined
	std::string GetStringRep(const std::vector<FactorType> factorsToPrint) const; //TODO not defined
	const Word& GetWord(size_t pos) const;

	TranslationOptionCollection* CreateTranslationOptionCollection() const;
};

std::ostream& operator<<(std::ostream& out,const ConfusionNet& cn);
#endif
