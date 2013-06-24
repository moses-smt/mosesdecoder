#pragma once

# include "SRILM-API.h"
# include "moses/FFState.h"
# include "moses/Manager.h"
# include <set>
# include <map>
# include <string>
# include <vector>

using namespace std;

namespace Moses
{

class osmState : public FFState
{
public:
  osmState();
  int Compare(const FFState& other) const;
  void saveState(int jVal, int eVal, vector <string> & hist , map <int , string> & gapVal);
  int getJ()const {return j;}
  int getE()const {return E;}
  map <int , string> getGap() const { return gap;}
  vector <string> getHistory()const {return history;}
  void print() const;
  std::string getName() const;
  void saveDelHistory(vector <string> & histVal){delHistory = histVal;}

protected:
  int j, E;
  std::map <int,std::string> gap;
  std::vector <std::string> history;
  std::vector <std::string> delHistory;
};

class osmHypothesis
{

	private:
	
	std::vector <std::string> history;
	std::vector <std::string> operations;	// List of operations required to generated this hyp ...
	std::map <int,std::string> gap;	// Maintains gap history ...
	int j;	// Position after the last source word generated ...
	int E; // Position after the right most source word so far generated ...

	int gapCount; // Number of gaps inserted ...
	int deletionCount;
	int openGapCount;
	int gapWidth;	
	double opProb;

	vector <string> currE;
	vector <string> currF;
	vector < pair < set <int> , set <int> > > ceptsInPhrase;
	set <int> targetNullWords;
	set <int> sourceNullWords;

	int closestGap(std::map <int,std::string> gap,int j1, int & gp);
	int firstOpenGap(std::vector <int> & coverageVector);
	std::string intToString(int);
	int  getOpenGaps();
	int isTranslationOperation(int j);
	void removeReorderingOperations();

	void getMeCepts ( set <int> & eSide , set <int> & fSide , map <int , vector <int> > & tS , map <int , vector <int> > & sT);

	public:

	osmHypothesis();
	~osmHypothesis(){};
	void generateOperations(int & startIndex, int j1 , int contFlag , WordsBitmap & coverageVector , std::string english , std::string german , std::set <int> & targetNullWords , std::vector <std::string> & currF);
	void generateDeleteOperations(std::string english, int currTargetIndex, std::set <int> doneTargetIndexes);
	void calculateOSMProb(Api & opPtr , int order);
	void computeOSMFeature(int startIndex , WordsBitmap & coverageVector , Api & ptrOp, int order);
	void constructCepts(vector <int> & align , int startIndex , int endIndex, int targetPhraseLength);
	void setPhrases(vector <string> & val1 , vector <string> & val2){currF = val1; currE = val2;}
	void setState(const FFState* prev_state);
	osmState * saveState();
	void print();
	void populateScores(vector <float> & scores);

};

} // namespace



