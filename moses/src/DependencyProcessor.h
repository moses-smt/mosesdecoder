#ifndef moses_DependencyProcessor_h
#define moses_DependencyProcessor_h

#include <string>
#include <vector>
#include "Factor.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "Util.h"
#include "WordsRange.h"
#include "ScoreProducer.h"
#include "FeatureFunction.h"
#include "FactorTypeSet.h"
#include "Sentence.h"

#include <list>

#include "TargetPhrase.h"

#include "../../moses-chart/src/ChartHypothesis.h"//gaoyang1011

namespace Moses {

    //gaoyang0801 remove this struct from bottom to top, and no longer the "not define a type" error
    struct DepInfoStruct
    {
        int lengthToTop;
        int headPos;
	std::vector<Word*> wordVec;//gaoyang1018
    };

    class DependencyProcessor {   

	typedef std::map< const Word*, float, WordComparer > SingleHash;
	typedef std::map< std::string, SingleHash > DoubleHash;	

    private:
	static const bool debugging = false;
	float m_reorderingBase;//gaoyang1024
	float m_treePenaltyBase;//gaoyang1025

	typedef int ReorderingType;
    	static const ReorderingType L = 0;  // left
    	static const ReorderingType R = 1; // right

        DoubleHash m_hash;

	std::set<std::string> m_featureFuncSet;

	static const int m_sentNumLimit = 3500;

	std::vector< FactorType > m_inFactors;
	std::vector< FactorType > m_outFactors;

	std::vector< std::vector<DepInfoStruct> > m_depInfoVector;

        float m_bias;



	ReorderingType GetOrientationType(const WordsRange &prev, const WordsRange &curr) const;

	void LoadDepOrientFile(const std::string &filePath);

        float GetFeatureScoreByWord(std::string featureName, const Word& word) const;

        ReorderingType GetOrientationTypeHier(const ChartRule &rule, const size_t depPosSourceInRule, const size_t headPosSourceInRule) const;//gaoyang1015

    public:

	DependencyProcessor(): m_reorderingBase(0), m_treePenaltyBase(0){}

	virtual ~DependencyProcessor();//0609 to see if different without virtual
	//~DependencyProcessor();

	void LoadFeatureFile(const std::string &filePath);

        void LoadSourcePreprocessedFile(const std::string &filePath);
	
	//gaoyang1025
	std::vector<size_t> ProcessHypo(const MosesChart::Hypothesis& cur_hypo, ScoreComponentCollection* out, std::vector <size_t> oldPosVector, std::vector <size_t> newPosVector) const;

	void SetTreePenaltyBase(const float tpb) { m_treePenaltyBase=tpb; }
	void SetReorderingBase(const float rb) { m_reorderingBase=rb; }


    };


}

#endif
