#pragma once

# include "SRILM-API.h"
#include "lm/model.hh"
# include "moses/FF/FFState.h"
# include "moses/Manager.h"
# include <set>
# include <map>
# include <string>
# include <vector>

namespace Moses
{

using namespace lm::ngram;

class osmState : public FFState
{
public:
  osmState(const State & val);
  int Compare(const FFState& other) const;
  void saveState(int jVal, int eVal, std::vector <std::string> & hist , std::map <int , std::string> & gapVal);
  int getJ()const {return j;}
  int getE()const {return E;}
  std::map <int , std::string> getGap() const { return gap;}
  std::vector <std::string> getHistory()const {return history;}

  State getLMState() const {return lmState;}	

  void print() const;
  std::string getName() const;
  void saveDelHistory(std::vector <std::string> & histVal){delHistory = histVal;}

protected:
  int j, E;
  std::map <int,std::string> gap;
  std::vector <std::string> history;
  std::vector <std::string> delHistory;
  State lmState;	
};

class osmHypothesis
{

	private:
	
	std::vector <std::string> history;
	std::vector <std::string> operations;	// List of operations required to generated this hyp ...
	std::map <int,std::string> gap;	// Maintains gap history ...
	int j;	// Position after the last source word generated ...
	int E; // Position after the right most source word so far generated ...
	State lmState; // KenLM's Model State ...

	int gapCount; // Number of gaps inserted ...
	int deletionCount;
	int openGapCount;
	int gapWidth;	
	double opProb;

	std::vector <std::string> currE;
	std::vector <std::string> currF;
	std::vector < std::pair < std::set <int> , std::set <int> > > ceptsInPhrase;
	std::set <int> targetNullWords;
	std::set <int> sourceNullWords;

	int closestGap(std::map <int,std::string> gap,int j1, int & gp);
	int firstOpenGap(std::vector <int> & coverageVector);
	std::string intToString(int);
	int  getOpenGaps();
	int isTranslationOperation(int j);
	void removeReorderingOperations();

	void getMeCepts ( std::set <int> & eSide , std::set <int> & fSide , std::map <int , std::vector <int> > & tS , std::map <int , std::vector <int> > & sT);

	public:

	osmHypothesis();
	~osmHypothesis(){};
	void generateOperations(int & startIndex, int j1 , int contFlag , WordsBitmap & coverageVector , std::string english , std::string german , std::set <int> & targetNullWords , std::vector <std::string> & currF);
	void generateDeleteOperations(std::string english, int currTargetIndex, std::set <int> doneTargetIndexes);
	void calculateOSMProb(Api & opPtr , int order);
	void calculateOSMProb(Model & ptrOp);
	void computeOSMFeature(int startIndex , WordsBitmap & coverageVector);
	void constructCepts(std::vector <int> & align , int startIndex , int endIndex, int targetPhraseLength);
	void setPhrases(std::vector <std::string> & val1 , std::vector <std::string> & val2){currF = val1; currE = val2;}
	void setState(const FFState* prev_state);
	osmState * saveState();
	void print();
	void populateScores(std::vector <float> & scores);

};

} // namespace



