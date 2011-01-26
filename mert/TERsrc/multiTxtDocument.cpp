#include "multiTxtDocument.h"

// #include <iostream>
// #include <boost/filesystem/fstream.hpp>
// #include <boost/archive/xml_oarchive.hpp>
// #include <boost/archive/xml_iarchive.hpp>
// #include <boost/serialization/nvp.hpp>

// helper functions to allow us to load and save sandwiches to/from xml
namespace TERCpp
{
    multiTxtDocument::multiTxtDocument()
    {
//       docType="";
//       setId="";
//       srcLang="";
//       tgtLang="";
    }
//     multiTxtDocument::multiTxtDocument ( string FileName )
//     {
// 	this=xmlStruct.copy_to_multiTxtDocument(FileName);
//     }
//     xmlStructure multiTxtDocument::getStructure()
//     {
// 	return xmlStruct;
//     }
//     string multiTxtDocument::getDocType()
//     {
//       return docType;
//     }
//     string multiTxtDocument::getSetId()
//     {
//       return setId;
//     }
//     string multiTxtDocument::getSrcLang()
//     {
//       return srcLang;
//     }
//     string multiTxtDocument::getTgtLang()
//     {
//       return tgtLang;
//     }
//     void multiTxtDocument::setDocType ( string s )
//     {
//       docType=s;
//     }
//     void multiTxtDocument::setSetId ( string s )
//     {
//       setId=s;
//     }
//     void multiTxtDocument::setSrcLang ( string s )
//     {
//       srcLang=s;
//     }
//     void multiTxtDocument::setTgtLang ( string s )
//     {
//       tgtLang=s;
//     }
    void multiTxtDocument::addDocument ( documentStructure doc )
    {
        documents.push_back ( doc );
    }
    documentStructure* multiTxtDocument::getLastDocument()
    {
        return & ( documents.at ( ( int ) documents.size() - 1 ) );
    }

    documentStructure* multiTxtDocument::getDocument ( string docId )
    {
        for ( int i = 0; i < ( int ) documents.size(); i++ )
        {
            if ( docId.compare ( documents.at ( i ).getDocId() ) == 0 )
            {
                return & ( documents.at ( i ) );
            }
        }
        cerr << "ERROR : multiTxtDocument::getDocument : document " << docId << " does not exist !" << endl;
        exit ( 0 );
    }

    void multiTxtDocument::loadFile ( string fileName, bool caseOn,  bool noPunct, bool debugMode, bool noTxtIds, bool tercomLike )
    {
        if ( multiTxtDocumentParams.debugMode )
        {
            cerr << "DEBUG tercpp : multiTxtDocument::loadFiles : loading files  " << endl << fileName << endl << "END DEBUG" << endl;
            cerr << "DEBUG tercpp : multiTxtDocument::loadFile : testing params  " << endl << Tools::printParams ( multiTxtDocumentParams ) << endl << "END DEBUG" << endl;
            cerr << "DEBUG tercpp : multiTxtDocument::loadFile : testing others params  " << endl << "caseOn : " << caseOn << endl << "noPunct : " << noPunct << endl << "debugMode : " << debugMode << endl << "noTxtIds : " << noTxtIds << endl << "tercomLike : " << tercomLike << endl << "END DEBUG" << endl;
        }

        ifstream fichierLoad ( fileName.c_str(), ios::in );
        string line;
        documentStructure l_doc;
        if ( fichierLoad )
        {
            int l_ids = 1;
            stringstream l_stream;
            while ( getline ( fichierLoad, line ) )
            {
                string l_key;
                string line_mod;
                l_stream.str ( "" );

                if ( noTxtIds )
                {
                    l_stream << l_ids;
                    l_key = l_stream.str();
                    line_mod = line;
                    l_ids++;
                }
                else
                {
                    l_key = line.substr ( line.rfind ( "(" ), line.size() - 1 );
                    line_mod = line.substr ( 0, line.rfind ( "(" ) - 1 );
                }
                if ( !tercomLike )
                {
                    if ( multiTxtDocumentParams.debugMode )
                    {
                        cerr << "DEBUG tercpp : multiTxtDocument::loadFiles : " << endl << "TERCOM AT FALSE " << endl << "END DEBUG" << endl;
                    }

                    line_mod = tokenizePunct ( line_mod );
                }
                if ( !caseOn )
                {
                    if ( multiTxtDocumentParams.debugMode )
                    {
                        cerr << "DEBUG tercpp : multiTxtDocument::loadFiles : " << endl << "CASEON AT FALSE " << endl << "END DEBUG" << endl;
                    }
                    line_mod = lowerCase ( line_mod );
                }
                if ( noPunct )
                {
                    if ( multiTxtDocumentParams.debugMode )
                    {
                        cerr << "DEBUG tercpp : multiTxtDocument::loadFiles : " << endl << "NOPUNCT AT TRUE " << endl << "END DEBUG" << endl;
                    }
                    if ( !tercomLike )
                    {
                        line_mod = removePunctTercom ( line_mod );
                    }
                    else
                    {
                        line_mod = removePunct ( line_mod );
                    }
                }
                if ( multiTxtDocumentParams.debugMode )
                {
                    cerr << "DEBUG multiTxtDocument::loadFile : line tokenized |" << line_mod << "|" << endl << "END DEBUG" << endl;
                }
                vector<string> vecDocLine = stringToVector ( line_mod, " " );
// 	  string l_key;
// 	  hashHypothesis.addValue(l_key,vecDocLine);
// 	  l_key=(string)vecDocLine.at((int)vecDocLine.size()-1);
// 	  vecDocLine.pop_back();
                if ( multiTxtDocumentParams.debugMode )
                {
                    cerr << "DEBUG tercpp multiTxtDocument::loadFile : " << l_key << "|" << vectorToString ( vecDocLine ) << "|" << endl << "Vector Size : " << vecDocLine.size() << endl << "Line length : " << ( int ) line_mod.length() << endl << "END DEBUG" << endl;
                }
//             hashHypothesis.addValue(l_key,vecDocLine);
                segmentStructure l_seg ( l_key, vecDocLine );
                l_doc.addSegments ( l_seg );
            }
//         Ref=line;
//         getline ( fichierHyp, line );
//         Hyp=line;
            fichierLoad.close();  // on ferme le fichier
            l_stream.str ( "" );
            l_stream << ( int ) documents.size();
            l_doc.setDocId ( l_stream.str() );
            addDocument ( l_doc );
            if ( multiTxtDocumentParams.debugMode )
            {
                cerr << "DEBUG multiTxtDocument::loadFile : document " << l_doc.getDocId() << " added !!!" << endl << "END DEBUG" << endl;
            }
        }
        else  // sinon
        {
            cerr << "ERROR : multiTxtDocument::loadFile : can't open file : " + fileName + " !" << endl;
            exit ( 0 );
        }
    }


// void save_sandwich(const multiTxtDocument &sw, const std::string &file_name);
// multiTxtDocument load_sandwich(const std::string &file_name);
// int callmultiTxtDocument()
// {
// 	// xml filename
// 	const std::string fn="JasonsSarnie.xml";
//
// 	// create a new sandwich and lets take a look at it!
// 	multiTxtDocument *s = new multiTxtDocument("Granary", "Brie", "Bacon", false); // mmmmm, Brie and bacon! ;)
// 	std::cout << "Created the following sandwich:" << std::endl;
// 	s->output();
//
// 	// Now lets save the sandwich out to an XML file....
// 	std::cout << std::endl << "Saving the sandwich to xml...." << std::endl;
// 	save_sandwich(*s, fn);
//
// 	// And then load it into another multiTxtDocument variable and take a look at what we've got
// 	std::cout << "Attempting to load the saved sandwich..." << std::endl;
// 	multiTxtDocument s2 = load_sandwich(fn);
// 	std::cout << "Contents of loaded multiTxtDocument:" << std::endl;
// 	s2.output();
//
// 	delete s;
// 	std::string dummy;
// 	std::getline(std::cin, dummy);
//
// }
    /*

    // Save a multiTxtDocument to XML...
    void save_sandwich(const multiTxtDocument &sw, const std::string &file_name)
    {
    	// Create a filestream object
    	boost::filesystem::fstream ofs(file_name, std::ios::trunc | std::ios::out);

    	// Now create an XML output file using our filestream
    	boost::archive::xml_oarchive xml(ofs);

    	// call serialization::make_nvp, passing our sandwich.
    	// make_nvp will eventually call the sandwich instance (sw) serialize function
    	// causing the contents of sw to be output to the xml file
    	xml << boost::serialization::make_nvp("multiTxtDocument", sw);
    }

    // The load function works in almost the exact same way as save_sandwich,
    // The only differences are:
    // 1. we create an XML input stream - the original example in AD's link created another xml_oarchive, causing a runtime error...doh!
    // 2. the call to make_nvp populates the sandwich instance(sw) which is then returned...
    multiTxtDocument load_sandwich(const std::string &file_name)
    {
    	multiTxtDocument sw;
    	boost::filesystem::fstream ifs(file_name, std::ios::binary | std::ios::in);
    	boost::archive::xml_iarchive xml(ifs);
    	xml >> boost::serialization::make_nvp("multiTxtDocument", sw);
    	return sw;
    }*/

    void multiTxtDocument::loadFiles ( string fileName, bool caseOn, bool noPunct, bool debugMode, bool noTxtIds, bool tercomLike )
    {
        vector<string> vecFiles = stringToVector ( fileName, "," );
        if ( multiTxtDocumentParams.debugMode )
        {
            cerr << "DEBUG tercpp : multiTxtDocument::loadFiles : loading files  " << endl << fileName << endl << "END DEBUG" << endl;

        }
        for ( int i = 0; i < ( int ) vecFiles.size(); i++ )
        {
            loadFile ( vecFiles.at ( i ), caseOn, noPunct, debugMode, noTxtIds, tercomLike );
        }
    }

    void multiTxtDocument::loadRefFile ( param p )
    {
        multiTxtDocumentParams = Tools::copyParam ( p );
        if ( multiTxtDocumentParams.debugMode )
        {
            cerr << "DEBUG tercpp : multiTxtDocument::loadRefFile : loading references  " << endl << multiTxtDocumentParams.referenceFile << endl << "END DEBUG" << endl;
        }
        loadFile ( multiTxtDocumentParams.referenceFile, multiTxtDocumentParams.caseOn, multiTxtDocumentParams.noPunct, multiTxtDocumentParams.debugMode, multiTxtDocumentParams.noTxtIds, multiTxtDocumentParams.tercomLike );
    }
    void multiTxtDocument::loadRefFiles ( param p )
    {
        multiTxtDocumentParams = Tools::copyParam ( p );
        if ( multiTxtDocumentParams.debugMode )
        {
            cerr << "DEBUG tercpp : multiTxtDocument::loadRefFiles : loading references  " << endl << multiTxtDocumentParams.referenceFile << endl << "END DEBUG" << endl;
        }
        loadFiles ( multiTxtDocumentParams.referenceFile, multiTxtDocumentParams.caseOn, multiTxtDocumentParams.noPunct, multiTxtDocumentParams.debugMode, multiTxtDocumentParams.noTxtIds, multiTxtDocumentParams.tercomLike );
    }
    void multiTxtDocument::loadHypFile ( param p )
    {
        multiTxtDocumentParams = Tools::copyParam ( p );
        multiTxtDocumentParams.tercomLike = false;
        if ( multiTxtDocumentParams.debugMode )
        {
            cerr << "DEBUG tercpp : multiTxtDocument::loadHypFile : loading hypothesis  " << endl << multiTxtDocumentParams.hypothesisFile << endl << "END DEBUG" << endl;
        }
        loadFile ( multiTxtDocumentParams.hypothesisFile, multiTxtDocumentParams.caseOn, multiTxtDocumentParams.noPunct, multiTxtDocumentParams.debugMode, multiTxtDocumentParams.noTxtIds, multiTxtDocumentParams.tercomLike );
    }
    void multiTxtDocument::loadHypFiles ( param p )
    {
        multiTxtDocumentParams = Tools::copyParam ( p );
        multiTxtDocumentParams.tercomLike = false;
        if ( multiTxtDocumentParams.debugMode )
        {
            cerr << "DEBUG tercpp : multiTxtDocument::loadHypFiles : loading hypothesis  " << endl << multiTxtDocumentParams.hypothesisFile << endl << "END DEBUG" << endl;
        }
        loadFile ( multiTxtDocumentParams.hypothesisFile, multiTxtDocumentParams.caseOn, multiTxtDocumentParams.noPunct, multiTxtDocumentParams.debugMode, multiTxtDocumentParams.noTxtIds, multiTxtDocumentParams.tercomLike );
    }

    int multiTxtDocument::getSize()
    {
        return ( int ) documents.size();
    }

}
