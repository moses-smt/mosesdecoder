#ifndef CUBEPRUNINGDATA_H_
#define CUBEPRUNINGDATA_H_

#include "WordsBitmap.h"
#include "WordsRange.h"
#include "Hypothesis.h"
#include "HypothesisStack.h"
#include "TranslationOption.h"


class CubePruningData
{
public:

	// keys for xData: coverage 
	std::map< WordsBitmap, std::vector< Hypothesis*> > xData;
	
	// key for yData: source words range
	std::map< WordsRange, TranslationOptionList > yData;
	
	CubePruningData();
	~CubePruningData();
	
//	void SaveData(Hypothesis *hypo, const vector< Hypothesis*> &coverageVec, TranslationOptionList &tol);

  void SaveSourceHypoColl(const WordsBitmap &wb, const vector< Hypothesis*> &coverageVec);
  bool SourceHypoCollExists(const WordsBitmap &wb ){	return ( xData.find(wb) != xData.end() ); }
	Hypothesis GetFirstHypothesis( const WordsBitmap &wb ) { return *(((*(xData.find(wb))).second)[0]); }

	void SaveTol(const WordsRange &wbr, TranslationOptionList &tol);
	bool TolExists(const WordsRange &wr ){	return ( yData.find(wr) != yData.end() ); }
	TranslationOption GetFirstTol( const WordsRange &wr ) { return *(((*(yData.find(wr))).second)[0]); }
	
	void DeleteData();
};

#endif /*CUBEPRUNINGDATA_H_*/



