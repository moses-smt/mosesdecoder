#include <cassert>
#include "FFState.h"
#include "StaticData.h"
#include "DiscriminativeReordering.h"
#include "WordsRange.h"
#include "TranslationOption.h"
#include <vector>
//gaoyang0419: add files for i/o
#include <fstream>
#include "InputFileStream.h"
#include "UserMessage.h"
#include "Manager.h" //gaoyang0702 to handle multi-threaded
#include "TreePenaltyProducer.h" //gaoyang1025
#include "../../moses-chart/src/ChartManager.h"//gaoyang1011
#include "DependencyProcessor.h"

namespace Moses {


    //gaoyang0710: this destructor is somehow not actually in use, don't know why...
    DependencyProcessor::~DependencyProcessor() {
        std::cerr << "calling destructor for DependencyProcessor" << std::endl;
    }



    void DependencyProcessor::LoadSourcePreprocessedFile(const std::string &filePath) {

        std::vector<std::string> filePathVec = Tokenize(filePath, "|||");

        for (size_t i = 0; i < filePathVec.size(); i++) {

	    std::cerr << "Loading source preprocessed file from " << filePathVec[i] << std::endl;

	    std::string fileType = Tokenize(filePathVec[i], ".").back();
	    //gaoyang1024, to handle filename of the type: eval02.chi.v2.2.dep-orient-dw-hw-hier

	    if (fileType.compare(0, 10, "dep-orient")==0)
		LoadDepOrientFile(filePathVec[i]);
        }

    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void DependencyProcessor::LoadDepOrientFile(const std::string &filePath) {

	std::cerr << "Load dep-orient file " << filePath << std::endl;
        InputFileStream inFile(filePath);

	FactorCollection &factorCollection = FactorCollection::Instance();
	const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();

	for (int i=0; i<m_sentNumLimit; i++) {
	    std::vector<DepInfoStruct> vec;
	    m_depInfoVector.push_back(vec);
	}

        // reading in data one line at a time
        std::string line;
        size_t lineNum = 0;

        while (getline(inFile, line)) {
            lineNum++;

            std::vector<std::string> token = Tokenize<std::string > (line);

            int sentId = Scan<int>(token[0]);
            int pos = Scan<int>(token[1]);
            int lengthToTop = Scan<int>(token[2]);
            int headPos = Scan<int>(token[3]);

	    assert(sentId < m_sentNumLimit);//gaoyang1018

	    DepInfoStruct depInfo;
	    depInfo.lengthToTop = lengthToTop;
	    depInfo.headPos = headPos;

	    int numFeat = token.size()-4;//gaoyang1018
	    for (size_t i=0; i<numFeat; i++)
	    {
		int tokenPos = 4+i;

	        // create word pointer
	        Word *word = new Word();
	        std::vector<std::string> factorString = Tokenize( token[tokenPos], factorDelimiter );
	        for (size_t i=0 ; i < m_inFactors.size() ; i++)
	        {
		    const FactorType& factorType = m_inFactors[i];
		    const Factor* factor = factorCollection.AddFactor( Input, factorType, factorString[i] );
		    word->SetFactor( factorType, factor );			
	        }
	        depInfo.wordVec.push_back(word);		
	    }

	    m_depInfoVector[sentId].push_back(depInfo);

        }

	if (debugging) {
	    for (size_t i=0; i<m_depInfoVector.size(); i++) {
	        for (size_t j=0; j<m_depInfoVector[i].size(); j++) {
                    std::cerr << "sentence:" << i << "\tpos:"<<j<<std::endl;
		    std::cerr<<"length-to-top:"<<m_depInfoVector[i][j].lengthToTop<<"\theadPos:"<<m_depInfoVector[i][j].headPos<< std::endl;
		    std::cerr<<"output wordVec\n";
		    for (size_t k=0; k<m_depInfoVector[i][j].wordVec.size(); k++)
		    {
		        std::cerr << "word*" << *m_depInfoVector[i][j].wordVec[k] << "*at*" << m_depInfoVector[i][j].wordVec[k]<<"\n";			
		    }
		}
	    }
        }


    }

    void DependencyProcessor::LoadFeatureFile(const std::string &str) {

	std::vector<std::string> spec = Tokenize<std::string>(str, " ");

	assert ( spec.size() == 2 );

	std::vector< std::string > factors = Tokenize(spec[0],"-");
	assert ( factors.size() == 2 );

	m_inFactors = Tokenize<FactorType>(factors[0],",");
	m_outFactors = Tokenize<FactorType>(factors[1],",");

	FactorCollection &factorCollection = FactorCollection::Instance();
	const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();

 
        std::vector<std::string> filePathVec = Tokenize(spec[1], "|||");

        for (size_t i = 0; i < filePathVec.size(); i++) {
            std::cerr << "Loading from feature file " << filePathVec[i] << std::endl;

            InputFileStream inFile(filePathVec[i]);
            std::string featureName;

	    std::vector< FactorType > factorVec;
	    FactorDirection direction;

            // reading in data one line at a time
            std::string line;
            size_t lineNum = 0;

            while (getline(inFile, line)) {
                lineNum++;

                if (lineNum == 1) {
                    featureName = line;
		    if (featureName.compare(0, 10, "DEP_ORIENT")==0){
			m_featureFuncSet.insert("DEP_ORIENT");
			factorVec = m_inFactors;
			direction = Input;}
                    continue;
                }

                if (lineNum == 2) {
                    m_bias = Scan<float>(Tokenize(line)[1]);
                    continue;
                }

		std::vector<std::string> token = Tokenize<std::string>(line, " ");

		if (token.size() != 2) // format checking
		{
			std::stringstream errorMessage;
			errorMessage << "Syntax error in file " << filePathVec[i] << " line :" << lineNum << std::endl;
			UserMessage::Add(errorMessage.str());
			abort();
		}

		// create word pointer
		Word *featWord = new Word();
		std::vector<std::string> factorString = Tokenize( token[0], factorDelimiter );

		for (size_t ind=0 ; ind < factorVec.size() ; ind++)
		{
			const FactorType& factorType = factorVec[ind];
			const Factor* factor = factorCollection.AddFactor( direction, factorType, factorString[ind] );
			featWord->SetFactor( factorType, factor );			
		}

                // maxent-trained score
                float score = Scan<float>(token[1]);

		// store feature in hash
		m_hash[featureName][featWord] = score;
            }
        }

        if (false) {

            std::cerr << "bias: " << m_bias << std::endl;
            for (DoubleHash::iterator it1 = m_hash.begin();
                    it1 != m_hash.end(); it1++) {
                std::cerr << "featureName: *" << it1->first << "*\n";
                for (SingleHash::iterator it2 = it1->second.begin(); it2 != it1->second.end();
                        it2++) {
                    std::cerr << "featWord*" << *it2->first << "*" << it2->second << "*\n";
		    std::cerr << "stored*" << it2->first << "*" << it2->second << "*\n";
                }
            }

            std::cerr << "featureFuncSet\n";
            for (std::set<std::string>::const_iterator it = m_featureFuncSet.begin(); it != m_featureFuncSet.end(); it++) {
                std::cerr << *it << std::endl;
            }
        }
    }

    float DependencyProcessor::GetFeatureScoreByWord(std::string featureName, const Word& word) const {
        if (debugging) {
	    std::cerr << "in getfeaturescorebyword\n";
	    std::cerr << "featureName:*" << featureName << "* word:*" << word << "*\n"; 
        }
        float result = 0;
            DoubleHash::const_iterator it = m_hash.find(featureName);
            if (it == m_hash.end()) {
                if (debugging) std::cerr << "no entry for feature: " << featureName << std::endl;
            } else {
                SingleHash::const_iterator it2 = (it->second).find(&word);
                if (it2 == (it->second).end()) {
                    if (debugging) std::cerr << word << " key is not found for feature: "<< featureName << "\n";
                } else {
                    if (debugging) std::cerr << "word:" << word << "wordaddress:"<<it2->first << " value: " << it2->second << std::endl;
                    result = it2->second;
                }
            }      
        if (debugging) std::cerr << "return from getfeaturescorebyword: " << result << std::endl;
        return result;
    }


    //gaoyang1006: for hierarchical model
    std::vector<size_t> DependencyProcessor::ProcessHypo(const MosesChart::Hypothesis& cur_hypo, ScoreComponentCollection* out, std::vector <size_t> oldPosVector, std::vector<size_t> newPosVector) const {

	if (debugging) std::cerr << "\nDepProc is called by ChartHypothesis\n";

        long sentenceId = cur_hypo.GetManager().GetSource().GetTranslationId();
	size_t sentenceLength = cur_hypo.GetManager().GetSource().GetSize();
        if (debugging) std::cerr << "sentence id: " << sentenceId << "\tlength: "<<sentenceLength<<std::endl; 

	const ChartRule &rule = cur_hypo.GetCurrChartRule();	
	const WordsRange &currSourceRange = cur_hypo.GetCurrSourceRange();

	float discReordering = 0;
	float treePenalty = 0;
	std::vector<size_t> unresolvedDepPosVector;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (debugging) std::cerr << "\nprocessing oldPosVector\n";

	for (size_t i=0; i<oldPosVector.size(); i++)
	{
		size_t depPosSourceAbsolute = oldPosVector[i];
		if (debugging) std::cerr << "depPosSourceAbsolute: " << depPosSourceAbsolute << std::endl; 

		if (depPosSourceAbsolute == sentenceLength-1) continue;

		size_t depPosSourceInRule = rule.GetSourcePosInRule(depPosSourceAbsolute);
		if (debugging) std::cerr << "depPosSourceInRule: "<<depPosSourceInRule<<std::endl;

		size_t headPosSourceAbsolute = m_depInfoVector[sentenceId][depPosSourceAbsolute].headPos;
		if (debugging) std::cerr << "headPosSourceAbsolute: "<<headPosSourceAbsolute<<std::endl;

		//note: if use size_t instead of int, negative values will be converted to a large positive value, don't know if it is c++ or moses spec.
		int diffProduct = ( headPosSourceAbsolute-currSourceRange.GetStartPos() ) * ( headPosSourceAbsolute-currSourceRange.GetEndPos() );
		if( diffProduct==0 || diffProduct<0)
		{
			if (debugging) std::cerr << "headPosSourceAbsolute is in the wordsRange: "<<currSourceRange<<std::endl;

			size_t headPosSourceInRule = rule.GetSourcePosInRule(headPosSourceAbsolute);	
			if (debugging) std::cerr << "headPosSourceInRule: "<<headPosSourceInRule<<std::endl;

			if (!rule.IsAligned(headPosSourceInRule))
			{	if (debugging) std::cerr << "headPosSourceInRule is not aligned, continue to the next posForDepScoring\n";		
				continue;
			}

			float featSum = m_bias;
			if (debugging) std::cerr << "m_bias is: "<<m_bias<<"\tnow summing up feat score\n";

		        for (size_t k=0; k<m_depInfoVector[sentenceId][depPosSourceAbsolute].wordVec.size(); k++)
		        {
		            if(debugging) std::cerr << "word*" << *m_depInfoVector[sentenceId][depPosSourceAbsolute].wordVec[k] << "*at*" << m_depInfoVector[sentenceId][depPosSourceAbsolute].wordVec[k]<<"\n";
			    featSum += GetFeatureScoreByWord("DEP_ORIENT", *m_depInfoVector[sentenceId][depPosSourceAbsolute].wordVec[k]);
			    if(debugging) std::cerr << "featSum updated: "<<featSum<<std::endl;	
		        }

       			float rightProb = 1 / (1 + exp(-featSum));
	
		        DependencyProcessor::ReorderingType reoType = GetOrientationTypeHier(rule, depPosSourceInRule, headPosSourceInRule);	

			float logProbUnweighted;					
			if (reoType==L)
			{
       				float leftProb = 1 - rightProb;
        			logProbUnweighted = log(leftProb);
    				if (debugging) std::cerr << "left orientation, unweighted logProb: "<< log(leftProb) << std::endl;
			}
			else
			{
            			logProbUnweighted = log(rightProb);
    				if (debugging) std::cerr << "right orientation, unweighted logProb: "<< log(rightProb) << std::endl;
			}
			if(debugging) std::cerr << "reordering-base: "<<m_reorderingBase << " exponent: "<<m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop << " resulting factor: "<< pow(m_reorderingBase, m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop)<<std::endl;
			if (debugging) std::cerr << "after weighting and flooring: " << FloorScore( logProbUnweighted*pow(m_reorderingBase, m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop) )<<std::endl;
			discReordering += FloorScore( logProbUnweighted*pow(m_reorderingBase, m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop) );		
		}
		else
		{	
			if (debugging) std::cerr << "headPosSourceAbsolute is not in the wordsRange: "<<currSourceRange<<std::endl;
			if (headPosSourceAbsolute != 9999) unresolvedDepPosVector.push_back(depPosSourceAbsolute);
		}
		
		if (debugging) std::cerr << "discReordering updated: " << discReordering << std::endl;
	}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (debugging) std::cerr << "\nprocessing newPosVector\n";

	for (size_t i=0; i<newPosVector.size(); i++)
	{
		size_t depPosSourceAbsolute = newPosVector[i];
		if (debugging) std::cerr << "depPosSourceAbsolute: " << depPosSourceAbsolute << std::endl; 

		if (depPosSourceAbsolute == sentenceLength-1) continue;

		size_t depPosSourceInRule = rule.GetSourcePosInRule(depPosSourceAbsolute);
		if (debugging) std::cerr << "depPosSourceInRule: "<<depPosSourceInRule<<std::endl;

		//checks the alignment of every terminal, including the sentence top word whose length-to-top is 0
		if (!rule.IsAligned(depPosSourceInRule)) 
		{
			if (debugging) std::cerr << "depPosSourceInRule is not aligned, length to top: "<< m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop<<" move to next depPosSourceInRule\n";
			//treePenalty += pow(m_treePenaltyBase, m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop);		
			continue;
		}

		size_t headPosSourceAbsolute = m_depInfoVector[sentenceId][depPosSourceAbsolute].headPos;
		if (debugging) std::cerr << "headPosSourceAbsolute: "<<headPosSourceAbsolute<<std::endl;

		//note: if use size_t instead of int, negative values will be converted to a large positive value, don't know if it is c++ or moses spec.
		int diffProduct = ( headPosSourceAbsolute-currSourceRange.GetStartPos() ) * ( headPosSourceAbsolute-currSourceRange.GetEndPos() );
		if( diffProduct==0 || diffProduct<0)
		{
			if (debugging) std::cerr << "headPosSourceAbsolute is in the wordsRange: "<<currSourceRange<<std::endl;

			size_t headPosSourceInRule = rule.GetSourcePosInRule(headPosSourceAbsolute);	
			if (debugging) std::cerr << "headPosSourceInRule: "<<headPosSourceInRule<<std::endl;

			if (!rule.IsAligned(headPosSourceInRule))
			{	if (debugging) std::cerr << "headPosSourceInRule is not aligned, continue to the next posForDepScoring\n";		
				continue;
			}

			float featSum = m_bias;
			if (debugging) std::cerr << "m_bias is: "<<m_bias<<"\tnow summing up feat score\n";

		        for (size_t k=0; k<m_depInfoVector[sentenceId][depPosSourceAbsolute].wordVec.size(); k++)
		        {
		            if(debugging) std::cerr << "word*" << *m_depInfoVector[sentenceId][depPosSourceAbsolute].wordVec[k] << "*at*" << m_depInfoVector[sentenceId][depPosSourceAbsolute].wordVec[k]<<"\n";
			    featSum += GetFeatureScoreByWord("DEP_ORIENT", *m_depInfoVector[sentenceId][depPosSourceAbsolute].wordVec[k]);
			    if(debugging) std::cerr << "featSum updated: "<<featSum<<std::endl;	
		        }

       			float rightProb = 1 / (1 + exp(-featSum));
	
		        DependencyProcessor::ReorderingType reoType = GetOrientationTypeHier(rule, depPosSourceInRule, headPosSourceInRule);	

			float logProbUnweighted;					
			if (reoType==L)
			{
       				float leftProb = 1 - rightProb;
        			logProbUnweighted = log(leftProb);
    				if (debugging) std::cerr << "left orientation, unweighted logProb: "<< log(leftProb) << std::endl;
			}
			else
			{
            			logProbUnweighted = log(rightProb);
    				if (debugging) std::cerr << "right orientation, unweighted logProb: "<< log(rightProb) << std::endl;
			}
			if(debugging) std::cerr << "reordering-base: "<<m_reorderingBase << " exponent: "<<m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop << " resulting factor: "<< pow(m_reorderingBase, m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop)<<std::endl;
			if (debugging) std::cerr << "after weighting and flooring: " << FloorScore( logProbUnweighted*pow(m_reorderingBase, m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop) )<<std::endl;
			discReordering += FloorScore( logProbUnweighted*pow(m_reorderingBase, m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop) );		
		}
		else
		{	
			if (debugging) std::cerr << "headPosSourceAbsolute is not in the wordsRange: "<<currSourceRange<<std::endl;
			if (headPosSourceAbsolute != 9999) 
			{
				treePenalty += pow(m_treePenaltyBase, m_depInfoVector[sentenceId][depPosSourceAbsolute].lengthToTop);	
				unresolvedDepPosVector.push_back(depPosSourceAbsolute);
			}
		}
		
		if (debugging) std::cerr << "discReordering updated: " << discReordering << std::endl;
	}

	//if (treePenalty > 100) treePenalty = 100;

	if (StaticData::Instance().GetUseTreePenalty())
	{
		if (debugging) std::cerr << "set treePenalty, finally: " << treePenalty << std::endl;
		const TreePenaltyProducer *tp = StaticData::Instance().GetTreePenaltyProducer();
		out->PlusEquals(tp, treePenalty);

	}

	if (StaticData::Instance().GetUseDiscriminativeReordering())
	{
		if (debugging) std::cerr << "set discReordering, finally: " << discReordering << std::endl;
		const DiscriminativeReordering *discR = StaticData::Instance().GetDiscriminativeReordering();
		out->PlusEquals(discR, discReordering);
	}	

	return unresolvedDepPosVector;
    }

    //gaoyang1015
    DependencyProcessor::ReorderingType DependencyProcessor::GetOrientationTypeHier(const ChartRule &rule, const size_t depPosSourceInRule, const size_t headPosSourceInRule) const {

	if(debugging) std::cerr << "in GetOrientationTypeHier, depPosSourceInRule: "<<depPosSourceInRule << " headPosSourceInRule: "<<headPosSourceInRule<<std::endl;
	if(debugging) std::cerr << "chartRule is: "<<rule << std::endl;

	DependencyProcessor::ReorderingType reoType = L;

	if(StaticData::Instance().GetOrientationByAllPos())
	{
		if (debugging) std::cerr << "orientationByAllPos\n";
		bool breakFlag = false;
		for (size_t i=0; i<rule.GetAlignmentVector(depPosSourceInRule).size(); i++)
		{
			if (breakFlag) break;
			size_t depPosTargetInRule = rule.GetAlignmentVector(depPosSourceInRule)[i];
			for (size_t j=0; j<rule.GetAlignmentVector(headPosSourceInRule).size(); j++)
			{	
				size_t headPosTargetInRule = rule.GetAlignmentVector(headPosSourceInRule)[j];
				int diffProduct = (depPosTargetInRule - headPosTargetInRule)*(depPosSourceInRule - headPosSourceInRule);
				if (diffProduct>0 || diffProduct==0) 
				{
					reoType = R;
					breakFlag = true;
					break;
				}
			}				
		}
	}

	else
	{
		if (debugging) std::cerr << "orientationByFirstPos\n";
		//by default use the OrientationByFirstPos criterion
		size_t depPosTargetInRule = rule.GetFirstTargetPosAlignedWith(depPosSourceInRule);
		size_t headPosTargetInRule = rule.GetFirstTargetPosAlignedWith(headPosSourceInRule);

		if(debugging) std::cerr << "depPosTargetInRule: "<<depPosTargetInRule<<std::endl;
		if(debugging) std::cerr << "headPosTargetInRule: "<<headPosTargetInRule<<std::endl;
     
		//bear in mind: size_t starts from 0, all negative numbers will be converted to a very large positive number
		//this may be due to c++ or moses special configuration
		//when negative value is needed, use int!   				
		int diffProduct = (depPosTargetInRule - headPosTargetInRule)*(depPosSourceInRule - headPosSourceInRule);
		if (diffProduct>0 || diffProduct==0) reoType = R;
	}

	if(debugging) std::cerr << "reoType: "<< reoType << std::endl;
	return reoType;	
    }


}
