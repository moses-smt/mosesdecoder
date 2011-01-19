#include "tools.h"

using namespace std;
using namespace boost::xpressive;
namespace Tools
{

string vectorToString ( vector<string> vec )
{
    string retour ( "" );
    for ( vector<string>::iterator vecIter=vec.begin();vecIter!=vec.end(); vecIter++ )
    {
	if (vecIter==vec.begin())
	{
	  retour+= ( *vecIter );
	}
	else
	{
	  retour+= "\t"+ ( *vecIter );
	}
    }
    return retour;
}

string vectorToString(vector< string > vec, string s)
{
    string retour ( "" );
    for ( vector<string>::iterator vecIter=vec.begin();vecIter!=vec.end(); vecIter++ )
    {
	if (vecIter==vec.begin())
	{
	  retour+= ( *vecIter );
	}
	else
	{
	  retour+= s+ ( *vecIter );
	}
    }
    return retour;

}

vector<string> subVector ( vector<string> vec, int start, int end )
{
    vector<string> retour;
    if ( start>end )
    {
        cerr << "ERREUR : TERcalc::subVector : end > start"<<endl;
        exit ( 0 );
    }
    for ( int i=start; ( ( i<end ) && ( i< ( int ) vec.size() ) ); i++ )
    {
        retour.push_back ( vec.at ( i ) );
    }
    return retour;
}
vector<string> copyVector ( vector<string> vec )
{
    vector<string> retour;
    for ( int i=0; i< ( int ) vec.size(); i++ )
    {
        retour.push_back ( vec.at ( i ) );
    }
    return retour;
}
vector<string> stringToVector ( string s, string tok )
{
    vector<string> to_return;
    string to_push ( "" );
    bool pushed=false;
    string::iterator sIt;
    for (sIt = s.begin(); sIt<s.end(); sIt++ )
    {
        pushed=false;
        for ( string::iterator sTok = tok.begin(); sTok<tok.end(); sTok++ )
        {
            if ( ( *sIt ) == ( *sTok ) )
            {
                to_return.push_back ( to_push );
                to_push="";
                pushed=true;
            }
        }
        if ( !pushed )
        {
            to_push.push_back ( ( *sIt ) );
        }
    }
    to_return.push_back ( to_push );
    return to_return;
}
vector<int> stringToVectorInt ( string s, string tok )
{
    vector<int> to_return;
    string to_push ( "" );
    bool pushed=false;
    string::iterator sIt;
    for (sIt = s.begin(); sIt<s.end(); sIt++ )
    {
        pushed=false;
        for ( string::iterator sTok = tok.begin(); sTok<tok.end(); sTok++ )
        {
            if ( ( *sIt ) == ( *sTok ) )
            {
                if ((int)to_push.length()>0)
                {
                    to_return.push_back ( atoi(to_push.c_str()) );
                }
                to_push="";
                pushed=true;
            }
        }
        if ( !pushed )
        {
            to_push.push_back ( ( *sIt ) );
        }
    }
    if ((int)to_push.length()>0)
    {
        to_return.push_back ( atoi(to_push.c_str()) );
    }
    return to_return;
}
string lowerCase(string str)
{
    for (int i=0;i<(int)str.size();i++)
    {
        if ((str[i] >= 0x41) && (str[i] <= 0x5A))
        {
            str[i] = str[i] + 0x20;
        }
    }
    return str;
}
string removePunct(string str)
{
    string str_mod=str;
    sregex rex;
    string replace;


    rex = sregex::compile( "^[ ]+" );
    replace= "";
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\"]" );
    replace=(" ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[,]" );
    replace= " ";
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "([^0-9])([\\.])([^0-9])" );
    replace=("$1 $3");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "([^0-9])([\\.])([^0-9])" );
    replace=("$1 $3");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "([^0-9])([\\.])([^0-9])" );
    replace=("$1 $3");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "([\\.]$)" );
    replace=(" ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\?]" );
    replace=(" ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\;]" );
    replace=(" ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\:]" );
    replace=(" ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\!]" );
    replace=(" ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\(]" );
    replace=(" ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\)]" );
    replace=(" ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[ ]+" );
    replace= " ";
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[ ]+$" );
    replace= "";
    str_mod = regex_replace( str_mod, rex, replace );

    return str_mod;
}
string tokenizePunct(string str)
{
    string str_mod=str;
    sregex rex = sregex::compile( "[,]" );
    string replace(" , ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "([^0-9][\\.][^0-9])" );
    replace=(" . ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\?]" );
    replace=(" ? ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\;]" );
    replace=(" ; ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\:]" );
    replace=(" : ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\!]" );
    replace=(" ! ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\(]" );
    replace=(" ( ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\\)]" );
    replace=(" ) ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[\"]" );
    replace=(" \" ");
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[ ]+" );
    replace= " ";
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "^[ ]+" );
    replace= "";
    str_mod = regex_replace( str_mod, rex, replace );

    rex = sregex::compile( "[ ]+$" );
    replace= "";
    str_mod = regex_replace( str_mod, rex, replace );

    return str_mod;
}

}
