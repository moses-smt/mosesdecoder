#include "sgmlDocument.h"

// #include <iostream>
// #include <boost/filesystem/fstream.hpp>
// #include <boost/archive/xml_oarchive.hpp>
// #include <boost/archive/xml_iarchive.hpp>
// #include <boost/serialization/nvp.hpp>

// helper functions to allow us to load and save sandwiches to/from xml
namespace TERCpp
{
    SGMLDocument::SGMLDocument()
    {
      docType="";
      setId="";
      srcLang="";
      tgtLang="";
    }
//     SGMLDocument::SGMLDocument ( string FileName )
//     {
// 	this=xmlStruct.copy_to_SGMLDocument(FileName);
//     }
//     xmlStructure SGMLDocument::getStructure()
//     {
// 	return xmlStruct;
//     }
    string SGMLDocument::getDocType()
    {
      return docType;
    }
    string SGMLDocument::getSetId()
    {
      return setId;
    }
    string SGMLDocument::getSrcLang()
    {
      return srcLang;
    }
    string SGMLDocument::getTgtLang()
    {
      return tgtLang;
    }
    void SGMLDocument::setDocType ( string s )
    {
      docType=s;
    }
    void SGMLDocument::setSetId ( string s )
    {
      setId=s;
    }
    void SGMLDocument::setSrcLang ( string s )
    {
      srcLang=s;
    }
    void SGMLDocument::setTgtLang ( string s )
    {
      tgtLang=s;
    }
    void SGMLDocument::addDocument ( documentStructure doc )
    {
      documents.push_back(doc);
    }
    documentStructure* SGMLDocument::getLastDocument()
    {
      return &(documents.at((int)documents.size()-1));
    }
    documentStructure* SGMLDocument::getFirstDocument()
    {
	  return &(documents.at(0));
    }
    int SGMLDocument::getSize()
    {
      return (int)documents.size();
    }
    documentStructure* SGMLDocument::getDocument(string docId)
    {
        for ( int i = 0; i < ( int ) documents.size(); i++ )
        {
            if ( docId.compare ( documents.at ( i ).getDocId() ) == 0 )
            {
                return & ( documents.at ( i ) );
            }
        }
        cerr << "ERROR : SGMLDocument::getDocument : document " << docId << " does not exist !" << endl;
        exit ( 0 );
    }





// void save_sandwich(const SGMLDocument &sw, const std::string &file_name);
// SGMLDocument load_sandwich(const std::string &file_name);
// int callSGMLDocument()
// {
// 	// xml filename
// 	const std::string fn="JasonsSarnie.xml";
// 
// 	// create a new sandwich and lets take a look at it!
// 	SGMLDocument *s = new SGMLDocument("Granary", "Brie", "Bacon", false); // mmmmm, Brie and bacon! ;)
// 	std::cout << "Created the following sandwich:" << std::endl;
// 	s->output(); 
// 
// 	// Now lets save the sandwich out to an XML file....
// 	std::cout << std::endl << "Saving the sandwich to xml...." << std::endl;
// 	save_sandwich(*s, fn);
// 
// 	// And then load it into another SGMLDocument variable and take a look at what we've got
// 	std::cout << "Attempting to load the saved sandwich..." << std::endl;
// 	SGMLDocument s2 = load_sandwich(fn);
// 	std::cout << "Contents of loaded SGMLDocument:" << std::endl;
// 	s2.output();
// 
// 	delete s;
// 	std::string dummy;
// 	std::getline(std::cin, dummy);
// 
// }
/*

// Save a SGMLDocument to XML...
void save_sandwich(const SGMLDocument &sw, const std::string &file_name)
{
	// Create a filestream object
	boost::filesystem::fstream ofs(file_name, std::ios::trunc | std::ios::out);
	
	// Now create an XML output file using our filestream
	boost::archive::xml_oarchive xml(ofs);

	// call serialization::make_nvp, passing our sandwich.
	// make_nvp will eventually call the sandwich instance (sw) serialize function
	// causing the contents of sw to be output to the xml file
	xml << boost::serialization::make_nvp("SGMLDocument", sw);
}

// The load function works in almost the exact same way as save_sandwich,
// The only differences are:
// 1. we create an XML input stream - the original example in AD's link created another xml_oarchive, causing a runtime error...doh!
// 2. the call to make_nvp populates the sandwich instance(sw) which is then returned...
SGMLDocument load_sandwich(const std::string &file_name)
{
	SGMLDocument sw;
	boost::filesystem::fstream ifs(file_name, std::ios::binary | std::ios::in);
	boost::archive::xml_iarchive xml(ifs);
	xml >> boost::serialization::make_nvp("SGMLDocument", sw);
	return sw;
}*/
  


}
