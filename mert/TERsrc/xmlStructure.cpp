#include "xmlStructure.h"

// The following class defines a hash function for strings


using namespace std;

namespace TERCpp
{

    // tutorial demo program

// ----------------------------------------------------------------------
// STDOUT dump and indenting utility functions
// ----------------------------------------------------------------------
// const unsigned int NUM_INDENTS_PER_SPACE=2;

    xmlStructure::xmlStructure()
    {
        NUM_INDENTS_PER_SPACE = 2;
    }

    const char * xmlStructure::getIndent ( unsigned int numIndents )
    {
        static const char * pINDENT = "                                      + ";
        static const unsigned int LENGTH = strlen ( pINDENT );
        unsigned int n = numIndents * NUM_INDENTS_PER_SPACE;
        if ( n > LENGTH )
            n = LENGTH;

        return &pINDENT[ LENGTH-n ];
    }

// same as getIndent but no "+" at the end
    const char * xmlStructure::getIndentAlt ( unsigned int numIndents )
    {
        static const char * pINDENT = "                                        ";
        static const unsigned int LENGTH = strlen ( pINDENT );
        unsigned int n = numIndents * NUM_INDENTS_PER_SPACE;
        if ( n > LENGTH )
            n = LENGTH;

        return &pINDENT[ LENGTH-n ];
    }

    int xmlStructure::dump_attribs_to_stdout ( TiXmlElement* pElement, unsigned int indent )
    {
        if ( !pElement )
            return 0;

        TiXmlAttribute* pAttrib = pElement->FirstAttribute();
        int i = 0;
        int ival;
        double dval;
        const char* pIndent = getIndent ( indent );
        printf ( "\n" );
        while ( pAttrib )
        {
            printf ( "%s%s: value=[%s]", pIndent, pAttrib->Name(), pAttrib->Value() );

            if ( pAttrib->QueryIntValue ( &ival ) == TIXML_SUCCESS )
                printf ( " int=%d", ival );
            if ( pAttrib->QueryDoubleValue ( &dval ) == TIXML_SUCCESS )
                printf ( " d=%1.1f", dval );
            printf ( "\n" );
            i++;
            pAttrib = pAttrib->Next();
        }
        return i;
    }

    void xmlStructure::dump_to_stdout ( TiXmlNode* pParent, unsigned int indent = 0 )
    {
        if ( !pParent )
            return;

        TiXmlNode* pChild;
        TiXmlText* pText;
        int t = pParent->Type();
        printf ( "%s", getIndent ( indent ) );
        int num;

        switch ( t )
        {
            case TiXmlNode::DOCUMENT:
                printf ( "Document" );
                break;

            case TiXmlNode::ELEMENT:
                printf ( "Element [%s]", pParent->Value() );
                num = dump_attribs_to_stdout ( pParent->ToElement(), indent + 1 );
                switch ( num )
                {
                    case 0:
                        printf ( " (No attributes)" );
                        break;
                    case 1:
                        printf ( "%s1 attribute", getIndentAlt ( indent ) );
                        break;
                    default:
                        printf ( "%s%d attributes", getIndentAlt ( indent ), num );
                        break;
                }
                break;

            case TiXmlNode::COMMENT:
                printf ( "Comment: [%s]", pParent->Value() );
                break;

            case TiXmlNode::UNKNOWN:
                printf ( "Unknown" );
                break;

            case TiXmlNode::TEXT:
                pText = pParent->ToText();
                printf ( "Text: [%s]", pText->Value() );
                break;

            case TiXmlNode::DECLARATION:
                printf ( "Declaration" );
                break;
            default:
                break;
        }
        printf ( "\n" );
        for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling() )
        {
            dump_to_stdout ( pChild, indent + 1 );
        }
    }

// load the named file and dump its structure to STDOUT
    void xmlStructure::dump_to_stdout ( const char* pFilename )
    {
        TiXmlDocument doc ( pFilename );
        bool loadOkay = doc.LoadFile();
        if ( loadOkay )
        {
            printf ( "\n%s:\n", pFilename );
            dump_to_stdout ( &doc ); // defined later in the tutorial
        }
        else
        {
            printf ( "Failed to load file \"%s\"\n", pFilename );
        }
    }
// Load the file and dump it into a SGMLDocument.
    SGMLDocument xmlStructure::dump_to_SGMLDocument ( string FileName )
    {

        TiXmlDocument doc ( FileName.c_str() );
        SGMLDocument to_return;
        bool isLoaded = doc.LoadFile();
        if ( isLoaded )
        {
            copy_to_SGMLDocument ( &to_return, &doc, ( unsigned int ) 0 );
        }
        else
        {
            cerr << "ERROR : xmlStructure::dump_to_SGMLDocument : Failed to load file " << FileName << endl;
            exit ( 0 );
        }
        return to_return;
    }

    void xmlStructure::copy_to_SGMLDocument ( SGMLDocument* sgmlDoc, TiXmlNode* pParent, unsigned int indent )
    {
        if ( !pParent )
            return;

        TiXmlNode* pChild;
        TiXmlText* pText;
        int t = pParent->Type();
//         printf ( "%s", getIndent ( indent ) );
//         int num;
        string elementValue;
        switch ( t )
        {
            case TiXmlNode::DOCUMENT:
//                 printf ( "Document" );
                break;

            case TiXmlNode::ELEMENT:
                printf ( "Element [%s]", pParent->Value() );
                elementValue = pParent->Value();
                if ( ( ( int ) elementValue.compare ( "refset" ) == 0 ) || ( ( int ) elementValue.compare ( "tstset" ) == 0 ) )
                {
                    sgmlDoc->setDocType ( elementValue );
                }
                else
                    if ( ( int ) elementValue.compare ( "doc" ) == 0 )
                    {
                        documentStructure tmp_doc;
                        sgmlDoc->addDocument ( tmp_doc );
                    }
                    else
                        if ( ( int ) elementValue.compare ( "seg" ) == 0 )
                        {
                            segmentStructure tmp_seg;
                            ( sgmlDoc->getLastDocument() )->addSegments ( tmp_seg );
                        }
                dump_attribs_to_SGMLDocuments ( sgmlDoc, pParent->ToElement(), indent + 1 );
//                 num = dump_attribs_to_stdout ( pParent->ToElement(), indent + 1 );
//                 switch ( num )
//                 {
//                     case 0:
//                         printf ( " (No attributes)" );
//                         break;
//                     case 1:
//                         printf ( "%s1 attribute", getIndentAlt ( indent ) );
//                         break;
//                     default:
//                         printf ( "%s%d attributes", getIndentAlt ( indent ), num );
//                         break;
//                 }
                break;

//             case TiXmlNode::COMMENT:
//                 printf ( "Comment: [%s]", pParent->Value() );
//                 break;
//
//             case TiXmlNode::UNKNOWN:
//                 printf ( "Unknown" );
//                 break;

            case TiXmlNode::TEXT:
                pText = pParent->ToText();
//                 printf ( "Text: [%s]", pText->Value() );
                if ( indent == 5 )
                {
                    documentStructure * l_tmp_doc = sgmlDoc->getLastDocument();
                    segmentStructure * l_tmp_seg = l_tmp_doc->getLastSegments();
                    string l_text = pText->Value();
                    l_tmp_seg->addContent ( l_text );
                }
                break;

//             case TiXmlNode::DECLARATION:
//                 printf ( "Declaration" );
//                 break;
            default:
                break;
        }
//         printf ( "\n" );
        for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling() )
        {
            copy_to_SGMLDocument ( sgmlDoc, pChild, indent + 1 );
        }
    }

    void xmlStructure::dump_attribs_to_SGMLDocuments ( SGMLDocument * sgmlDoc, TiXmlElement* pElement, unsigned int indent )
    {
        if ( !pElement )
            return;
        TiXmlAttribute* pAttrib = pElement->FirstAttribute();
//         int i = 0;
//         int ival;
//         double dval;
//         const char* pIndent = getIndent ( indent );
//         printf ( "\n" );
        while ( pAttrib )
        {
            string attribut = pAttrib->Name();
            switch ( indent )
            {
                case 1 :
                {
                    if ( attribut.compare ( "setid" ) == 0 )
                    {
                        sgmlDoc->setSetId ( pAttrib->Value() );
                    }
                    if ( attribut.compare ( "srclang" ) == 0 )
                    {
                        sgmlDoc->setSrcLang ( pAttrib->Value() );
                    }
                    if ( attribut.compare ( "tgtlang" ) == 0 )
                    {
                        sgmlDoc->setTgtLang ( pAttrib->Value() );
                    }
                }
                break;

                case 2:
                {
                    documentStructure * tmp_doc_bis = sgmlDoc->getLastDocument();
                    if ( attribut.compare ( "docid" ) == 0 )
                    {
                        tmp_doc_bis->setDocId ( pAttrib->Value() );
                    }
                    if ( attribut.compare ( "sysid" ) == 0 )
                    {
                        tmp_doc_bis->setSysId ( pAttrib->Value() );
                    }
                }
                break;

                case 4:
                {
                    documentStructure * l_tmp_doc = sgmlDoc->getLastDocument();
                    segmentStructure * l_tmp_seg = l_tmp_doc->getLastSegments();
                    if ( attribut.compare ( "id" ) == 0 )
                    {
                        l_tmp_seg->setSegId ( pAttrib->Value() );
                    }
// 		else
// 		if (attribut.compare("Text")==0)
// 		{
// 		  tmp_seg.addContent(pAttrib->Value());
// 		}
                }
                break;
                default:
                    break;

            }
//             printf ( "%s%s: value=[%s]", pIndent, pAttrib->Name(), pAttrib->Value() );

//             if ( pAttrib->QueryIntValue ( &ival ) == TIXML_SUCCESS )
//                 printf ( " int=%d", ival );
//             if ( pAttrib->QueryDoubleValue ( &dval ) == TIXML_SUCCESS )
//                 printf ( " d=%1.1f", dval );
//             printf ( "\n" );
//             i++;
            pAttrib = pAttrib->Next();
        }
//         return i;
    }


//     std::size_t hashValue(std::string key){}

}

