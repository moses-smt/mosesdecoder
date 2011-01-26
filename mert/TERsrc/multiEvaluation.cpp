#include "multiEvaluation.h"


// #include <iostream>
// #include <boost/filesystem/fstream.hpp>
// #include <boost/archive/xml_oarchive.hpp>
// #include <boost/archive/xml_iarchive.hpp>
// #include <boost/serialization/nvp.hpp>

// helper functions to allow us to load and save sandwiches to/from xml

namespace TERCpp
{

    multiEvaluation::multiEvaluation()
    {
        evalParameters.debugMode = false;
        evalParameters.caseOn = false;
        evalParameters.noPunct = false;
        evalParameters.normalize = false;
        evalParameters.tercomLike = false;
        evalParameters.sgmlInputs = false;
        evalParameters.noTxtIds = false;
// 	referencesTxt=new multiTxtDocument();
// 	hypothesisTxt=new documentStructure();
    }

    multiEvaluation::multiEvaluation ( param p )
    {
        evalParameters.debugMode = false;
        evalParameters.caseOn = false;
        evalParameters.noPunct = false;
        evalParameters.normalize = false;
        evalParameters.tercomLike = false;
        evalParameters.sgmlInputs = false;
        evalParameters.noTxtIds = false;
	
        evalParameters = Tools::copyParam ( p );
// 	referencesTxt=new multiTxtDocument();
// 	hypothesisTxt=new documentStructure();
    }

    void multiEvaluation::addReferences()
    {
        referencesTxt.loadRefFiles ( evalParameters );
    }
// void multiEvaluation::addReferences(vector< string > vecRefecrences)
// {
//     for (int i=0; i< (int) vecRefecrences.size(); i++)
//     {
//         referencesTxt.loadFile(vecRefecrences.at(i));
//     }
// }


    void multiEvaluation::setHypothesis()
    {
        multiTxtDocument l_multiTxtTmp;
        l_multiTxtTmp.loadHypFile ( evalParameters );
        hypothesisTxt = (*(l_multiTxtTmp.getDocument ( "0" )));
    }
    void multiEvaluation::setParameters ( param p )
    {
        evalParameters = Tools::copyParam ( p );
    }

    void multiEvaluation::launchTxtEvaluation()
    {
	if (evalParameters.debugMode)
	{
		    cerr <<"DEBUG tercpp : multiEvaluation::launchTxtEvaluation : before testing references and hypothesis size  "<<endl<<"END DEBUG"<<endl;
	}
	  
        if ( referencesTxt.getSize() == 0 )
        {
            cerr << "ERROR : multiEvaluation::launchTxtEvaluation : there is no references" << endl;
            exit ( 0 );
        }
        if ( hypothesisTxt.getSize() == 0 )
        {
            cerr << "ERROR : multiEvaluation::launchTxtEvaluation : there is no hypothesis" << endl;
            exit ( 0 );
        }
	if (evalParameters.debugMode)
	{
		    cerr <<"DEBUG tercpp : multiEvaluation::launchTxtEvaluation : testing references and hypothesis size  "<<endl<<" references size : "<<  referencesTxt.getSize() << endl << " hypothesis size : "<<  hypothesisTxt.getSize() << endl<<"END DEBUG"<<endl;
	}
	  
        int incDocRefences = 0;
        stringstream l_stream;
        vector<float> editsResults;
        vector<float> wordsResults;
        int tot_ins = 0;
        int tot_del = 0;
        int tot_sub = 0;
        int tot_sft = 0;
        int tot_wsf = 0;
        float tot_err = 0;
        float tot_wds = 0;
//         vector<stringInfosHasher> setOfHypothesis = hashHypothesis.getHashMap();
        ofstream outputSum ( ( evalParameters.hypothesisFile + ".output.sum.log" ).c_str() );
        outputSum << "Hypothesis File: " + evalParameters.hypothesisFile + "\nReference File: " + evalParameters.referenceFile + "\n" + "Ave-Reference File: " << endl;
        char outputCharBuffer[200];
        sprintf ( outputCharBuffer, "%19s | %4s | %4s | %4s | %4s | %4s | %6s | %8s | %8s", "Sent Id", "Ins", "Del", "Sub", "Shft", "WdSh", "NumEr", "NumWd", "TER");
        outputSum << outputCharBuffer << endl;
        outputSum << "-------------------------------------------------------------------------------------" << endl;
        for ( incDocRefences = 0; incDocRefences < referencesTxt.getSize();incDocRefences++ )
        {
            l_stream.str ( "" );
            l_stream << incDocRefences;
            documentStructure l_reference = (*(referencesTxt.getDocument ( l_stream.str() )));
            evaluate ( l_reference, hypothesisTxt );
        }
        for ( vector<segmentStructure>::iterator segHypIt = hypothesisTxt.getSegments()->begin(); segHypIt != hypothesisTxt.getSegments()->end(); segHypIt++ )
        {
            terAlignment l_result = segHypIt->getAlignment();
	    string bestDocId = segHypIt->getBestDocId();
	    string l_id=segHypIt->getSegId();
            editsResults.push_back(l_result.numEdits);
            wordsResults.push_back(l_result.numWords);
            l_result.scoreDetails();
            tot_ins += l_result.numIns;
            tot_del += l_result.numDel;
            tot_sub += l_result.numSub;
            tot_sft += l_result.numSft;
            tot_wsf += l_result.numWsf;
            tot_err += l_result.numEdits;
            tot_wds += l_result.numWords;

            char outputCharBufferTmp[200];
            sprintf(outputCharBufferTmp, "%19s | %4d | %4d | %4d | %4d | %4d | %6.1f | %8.3f | %8.3f",(l_id+":"+bestDocId).c_str(), l_result.numIns, l_result.numDel, l_result.numSub, l_result.numSft, l_result.numWsf, l_result.numEdits, l_result.numWords, l_result.score()*100.0);
            outputSum<< outputCharBufferTmp<<endl;

            if (evalParameters.debugMode)
            {
                cerr <<"DEBUG tercpp : multiEvaluation::launchTxtEvaluation : Evaluation "<<endl<< l_result.toString() <<endl<<"END DEBUG"<<endl;
            }

        }

        cout << "Total TER: " << scoreTER ( editsResults, wordsResults );
        char outputCharBufferTmp[200];
        outputSum << "-------------------------------------------------------------------------------------" << endl;
        sprintf ( outputCharBufferTmp, "%19s | %4d | %4d | %4d | %4d | %4d | %6.1f | %8.3f | %8.3f", "TOTAL", tot_ins, tot_del, tot_sub, tot_sft, tot_wsf, tot_err, tot_wds, tot_err*100.0 / tot_wds );
        outputSum << outputCharBufferTmp << endl;
        outputSum.close();


    }
    void multiEvaluation::evaluate ( documentStructure& docStructReference, documentStructure& docStructhypothesis )
    {
	if (evalParameters.debugMode)
	{
		    cerr <<"DEBUG tercpp : multiEvaluation::evaluate : launching evaluate on  "<<endl<<" references size : "<<  docStructReference.getSize() << endl << " hypothesis size : "<<  docStructhypothesis.getSize() << endl<<"END DEBUG"<<endl;
	}
	if (evalParameters.debugMode)
	{
		    cerr <<"DEBUG tercpp : multiEvaluation::evaluate : testing hypothesis "<<endl;
		    cerr <<" segId : "<<  docStructhypothesis.getSegments()->at(0).getSegId() << endl<<"END DEBUG"<<endl;
	}

        for ( vector<segmentStructure>::iterator segHypIt = docStructhypothesis.getSegments()->begin(); segHypIt != docStructhypothesis.getSegments()->end(); segHypIt++ )
        {
// 	  cerr << "************************************************************************************************************************************************************************************** 1 " << (docStructhypothesis.getSegments()->at(0)).toString()<<endl;
            terCalc l_evalTER;
// 	  cerr << "************************************************************************************************************************************************************************************** 2"<<endl;
// 	  (*segHypIt).getSegId() ;
// 	  cerr << "************************************************************************************************************************************************************************************** 3"<<endl;
            segmentStructure * l_segRef = docStructReference.getSegment ( segHypIt->getSegId() );
// 	  cerr << "************************************************************************************************************************************************************************************** 4"<<endl;
// 	    exit(0);
            terAlignment l_result = l_evalTER.TER ( segHypIt->getContent(), l_segRef->getContent() );
            l_segRef->setAlignment ( l_result );
	    if (evalParameters.debugMode)
	    {
			cerr <<"DEBUG tercpp : multiEvaluation::evaluate : testing   "<<endl<<" hypothesis : "<<  segHypIt->getSegId() <<endl;
			cerr << "hypothesis score : "<<  segHypIt->getAlignment().score() <<endl;
			cerr << "BestDoc Id : "<<  segHypIt->getBestDocId() <<endl;
			cerr << "new score : "<<  l_result.score()  <<endl;
			cerr << "new BestDoc Id : "<< docStructReference.getDocId()  <<endl;
			cerr << endl<<"END DEBUG"<<endl;
	    }
	    if ((segHypIt->getAlignment().numWords == 0) && (segHypIt->getAlignment().numEdits == 0 ))
	    {
                segHypIt->setAlignment ( l_result );
                segHypIt->setBestDocId ( docStructReference.getDocId() );
	    }
	    else if ( l_result.score() < segHypIt->getAlignment().score() )
            {
                segHypIt->setAlignment ( l_result );
                segHypIt->setBestDocId ( docStructReference.getDocId() );
            }
        }
	if (evalParameters.debugMode)
	{
		    cerr <<"DEBUG tercpp : multiEvaluation::evaluate :    "<<endl<<"End of function"<<endl<<"END DEBUG"<<endl;
	}
// 	for (incSegHypothesis=0; incSegHypothesis< getSize();incSegHypothesis++)
// 	{
// 	  docStructhypothesis->getSegments()
// 	}
    }

    string multiEvaluation::scoreTER ( vector<float> numEdits, vector<float> numWords )
    {
        vector<float>::iterator editsIt = numEdits.begin();
        vector<float>::iterator wordsIt = numWords.begin();
        if ( numWords.size() != numEdits.size() )
        {
            cerr << "ERROR : tercpp:score, diffrent size of hyp and ref" << endl;
            exit ( 0 );
        }

        double editsCount = 0.0;
        double wordsCount = 0.0;
        while ( editsIt != numEdits.end() )
        {
            editsCount += ( *editsIt );
            wordsCount += ( *wordsIt );
            editsIt++;
            wordsIt++;
        }
        stringstream output;

        if ( ( wordsCount <= 0.0 ) && ( editsCount > 0.0 ) )
        {
            output <<  1.0 << " (" << editsCount << "/" << wordsCount << ")" << endl;
        }
        else
            if ( wordsCount <= 0.0 )
            {
                output <<  0.0 << " (" << editsCount << "/" << wordsCount << ")" << endl;
            }
            else
            {
//       return editsCount/wordsCount;
                output <<  editsCount / wordsCount << " (" << editsCount << "/" << wordsCount << ")" << endl;
            }
        return output.str();
    }


}
