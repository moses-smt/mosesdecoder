/*
 * Generic hashmap manipulation functions
 */
#ifndef __XMLSTRUCTURE_H__
#define __XMLSTRUCTURE_H__

#include "sgmlDocument.h"
#include "documentStructure.h"
#include "stdio.h"
#include <iostream>
#include <string>
#include "tinyxml.h"

using namespace std;

namespace TERCpp
{
    class xmlStructure
    {
        private:
            unsigned int NUM_INDENTS_PER_SPACE;
//             void dump_attribs_to_SGMLDocuments ( SGMLDocument* arg1, const TiXmlElement* arg2 );
            void dump_attribs_to_SGMLDocuments ( SGMLDocument* sgmlDoc, TiXmlElement* pElement, unsigned int indent );
        public:
            xmlStructure();
            const char * getIndent ( unsigned int numIndents );
            const char * getIndentAlt ( unsigned int numIndents );
            int dump_attribs_to_stdout ( TiXmlElement* pElement, unsigned int indent );
            void dump_to_stdout ( TiXmlNode* pParent, unsigned int indent );
            void dump_to_stdout ( const char* pFilename );
            void copy_to_SGMLDocument ( SGMLDocument* sgmlDoc , TiXmlNode* pParent, unsigned int indent );
            SGMLDocument dump_to_SGMLDocument ( string FileName );
	    param xmlParams;
    };


}


#endif
