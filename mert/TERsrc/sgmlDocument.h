#ifndef __SGML_DOCUMENT_H__
#define __SGML_DOCUMENT_H__

#include "documentStructure.h"
// #include "xmlStructure.h"

#include <iostream>
#include <string>
namespace TERCpp
{

    class SGMLDocument
    {
        public:
            SGMLDocument();
// 		SGMLDocument(string FileName);
// 		SGMLDocument(const std::string &bread, const std::string &cheese, const std::string &meat, const bool pickle):
// 			m_bread(bread), m_cheese(cheese), m_meat(meat), m_pickle(pickle){};
// 		~SGMLDocument(){};

// 		void output()
// 		{
// 			std::cout << "Bread = " << m_bread << ", Cheese = " << m_cheese <<
// 				", Meat = " << m_meat << ", Has Pickle = " << m_pickle << std::endl;
//
// 		}
            void setDocType ( string s );
            void setSetId ( string s );
            void setSrcLang ( string s );
            void setTgtLang ( string s );
            string getDocType();
            string getSetId();
            string getSrcLang();
            string getTgtLang();
// 		xmlStructure getStructure();
            void addDocument ( documentStructure doc );
            documentStructure* getLastDocument();

        private:
            string docType;
            string setId;
            string srcLang;
            string tgtLang;
// 		xmlStructure xmlStruct;
            vector<documentStructure> documents;
// 		std::string m_bread, m_cheese, m_meat;
// 		bool m_pickle;
//
// 	// declare the boost::serialization::access class as a friend of SGMLDocument
// 	friend class boost::serialization::access;
// 	// Create a serialize function for serialization::access to use, I guess you could regard this as a kind of callback function!
// 	template<class archive>
// 	void serialize(archive& ar, const unsigned int version)
// 	{
// 		// Note: As explained in the original tut. the & operator is overridden in boost to use
// 		// << or >> depending on the direction of the data (read/write)
// 		using boost::serialization::make_nvp;
// 		ar & make_nvp("Bread", m_bread);
// 		ar & make_nvp("Cheese", m_cheese);
// 		ar & make_nvp("Meats", m_meat);
// 		ar & make_nvp("HasPickle", m_pickle);
// 		// Also note: strings in the first parameter of make_nvp cannot contain spaces!
// 	}
    };
}
#endif //SANDWICH_DEFINED
