// $Id$
#ifndef CONFUSIONNET_H_
#define CONFUSIONNET_H_
#include <vector>
#include <iostream>
#include "Word.h"
class FactorCollection;

class ConfusionNet {
 public: 
	typedef std::vector<std::pair<Word,float> > Column;

 private:
	std::vector<Column> data;
	FactorCollection *m_factorCollection;
 public:
	ConfusionNet(FactorCollection* p=0);

	void SetFactorCollection(FactorCollection*);

	const Column& GetColumn(size_t i) const {assert(i<data.size());return data[i];}
	const Column& operator[](size_t i) const {return GetColumn(i);}

	bool empty() const {return data.empty();}
	size_t size() const {return data.size();}
	void clear() {data.clear();}

	bool Read(std::istream&,const std::vector<FactorType>& factorOrder,int format=0);
	void Print(std::ostream&) const;

 private:
	bool ReadFormat0(std::istream&,const std::vector<FactorType>& factorOrder);
	bool ReadFormat1(std::istream&,const std::vector<FactorType>& factorOrder);
	void String2Word(const std::string& s,Word& w,const std::vector<FactorType>& factorOrder);
};

#endif
