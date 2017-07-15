/*********************************
tercpp: an open-source Translation Edit Rate (TER) scorer tool for Machine Translation.

Copyright 2010-2013, Christophe Servan, LIUM, University of Le Mans, France
Contact: christophe.servan@lium.univ-lemans.fr

The tercpp tool and library are free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the licence, or
(at your option) any later version.

This program and library are distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**********************************/
#include "tools.h"

using namespace std;
using namespace boost::xpressive;
namespace TERCPPNS_Tools
{

string vectorToString ( vector<string> vec )
{
  string retour ( "" );
  for ( vector<string>::iterator vecIter = vec.begin(); vecIter != vec.end(); vecIter++ ) {
    if ( vecIter == vec.begin() ) {
      retour += ( *vecIter );
    } else {
      retour += "\t" + ( *vecIter );
    }
  }
  return retour;
}
string vectorToString ( vector<char> vec )
{
  stringstream retour;
  retour.str("");
  for ( vector<char>::iterator vecIter = vec.begin(); vecIter != vec.end(); vecIter++ ) {
    if ( vecIter == vec.begin() ) {
      retour << ( *vecIter );
    } else {
      retour << "\t" << ( *vecIter );
    }
  }
  return retour.str();
}
string vectorToString ( vector<int> vec )
{
  stringstream retour;
  retour.str("");
  for ( vector<int>::iterator vecIter = vec.begin(); vecIter != vec.end(); vecIter++ ) {
    if ( vecIter == vec.begin() ) {
      retour << ( *vecIter );
    } else {
      retour << "\t" << ( *vecIter );
    }
  }
  return retour.str();
}
string vectorToString ( vector<int> * vec )
{
  stringstream retour;
  retour.str("");
  for ( vector<int>::iterator vecIter = vec->begin(); vecIter != vec->end(); vecIter++ ) {
    if ( vecIter == vec->begin() ) {
      retour << ( *vecIter );
    } else {
      retour << "\t" << ( *vecIter );
    }
  }
  return retour.str();
}

string vectorToString ( vector< string > vec, string s )
{
  string retour ( "" );
  for ( vector<string>::iterator vecIter = vec.begin(); vecIter != vec.end(); vecIter++ ) {
    if ( vecIter == vec.begin() ) {
      retour += ( *vecIter );
    } else {
      retour += s + ( *vecIter );
    }
  }
  return retour;

}

string vectorToString ( vector< char > vec, string s )
{
  stringstream retour;
  retour.str("");
  for ( vector<char>::iterator vecIter = vec.begin(); vecIter != vec.end(); vecIter++ ) {
    if ( vecIter == vec.begin() ) {
      retour << ( *vecIter );
    } else {
      retour << s << ( *vecIter );
    }
  }
  return retour.str();

}

string vectorToString ( vector< int > vec, string s )
{
  stringstream retour;
  retour.str("");
  for ( vector<int>::iterator vecIter = vec.begin(); vecIter != vec.end(); vecIter++ ) {
    if ( vecIter == vec.begin() ) {
      retour << ( *vecIter );
    } else {
      retour <<  s << ( *vecIter );
    }
  }
  return retour.str();

}

string vectorToString ( vector< bool > vec, string s )
{
  stringstream retour;
  retour.str("");
  for ( vector<bool>::iterator vecIter = vec.begin(); vecIter != vec.end(); vecIter++ ) {
    if ( vecIter == vec.begin() ) {
      retour << ( *vecIter );
    } else {
      retour <<  s << ( *vecIter );
    }
  }
  return retour.str();

}
string vectorToString ( char* vec, string s , int taille)
{
  stringstream retour;
  retour.str("");
  int l_i;
  for ( l_i=0; l_i < taille ; l_i++) {
    if ( l_i == 0 ) {
      retour << vec[l_i];
    } else {
      retour << s << vec[l_i];
    }
  }
  return retour.str();

}

string vectorToString ( int* vec, string s , int taille)
{
  stringstream retour;
  retour.str("");
  int l_i;
  for ( l_i=0; l_i < taille ; l_i++) {
    if ( l_i == 0 ) {
      retour << vec[l_i];
    } else {
      retour << s << vec[l_i];
    }
  }
  return retour.str();

}

string vectorToString ( bool* vec, string s , int taille)
{
  stringstream retour;
  retour.str("");
  int l_i;
  for ( l_i=0; l_i < taille ; l_i++) {
    if ( l_i == 0 ) {
      retour << vec[l_i];
    } else {
      retour << s << vec[l_i];
    }
  }
  return retour.str();

}

string vectorToString ( vector<bool>* vec, string s , int taille)
{
  stringstream retour;
  retour.str("");
  int l_i;
  for ( l_i=0; l_i < taille ; l_i++) {
    if ( l_i == 0 ) {
      retour << vec->at(l_i);
    } else {
      retour << s << vec->at(l_i);
    }
  }
  return retour.str();

}

string vectorToString ( vector<int>* vec, string s , int taille)
{
  stringstream retour;
  retour.str("");
  int l_i;
  for ( l_i=0; l_i < taille ; l_i++) {
    if ( l_i == 0 ) {
      retour << vec->at(l_i);
    } else {
      retour << s << vec->at(l_i);
    }
  }
  return retour.str();

}



vector<string> subVector ( vector<string> vec, int start, int end )
{
  vector<string> retour;
  if ( start > end ) {
    cerr << "ERREUR : TERcalc::subVector : end > start" << endl;
    exit ( 0 );
  }
  for ( int i = start; ( ( i < end ) && ( i < ( int ) vec.size() ) ); i++ ) {
    retour.push_back ( vec.at ( i ) );
  }
  return retour;
}

vector<int> subVector ( vector<int> vec, int start, int end )
{
  vector<int> retour;
  if ( start > end ) {
    cerr << "ERREUR : TERcalc::subVector : end > start" << endl;
    exit ( 0 );
  }
  for ( int i = start; ( ( i < end ) && ( i < ( int ) vec.size() ) ); i++ ) {
    retour.push_back ( vec.at ( i ) );
  }
  return retour;
}

vector<float> subVector ( vector<float> vec, int start, int end )
{
  vector<float> retour;
  if ( start > end ) {
    cerr << "ERREUR : TERcalc::subVector : end > start" << endl;
    exit ( 0 );
  }
  for ( int i = start; ( ( i < end ) && ( i < ( int ) vec.size() ) ); i++ ) {
    retour.push_back ( vec.at ( i ) );
  }
  return retour;
}

vector<string> copyVector ( vector<string> vec )
{
  vector<string> retour;
  for ( int i = 0; i < ( int ) vec.size(); i++ ) {
    retour.push_back ( vec.at ( i ) );
  }
  return retour;
}
vector<int> copyVector ( vector<int> vec )
{
  vector<int> retour;
  for ( int i = 0; i < ( int ) vec.size(); i++ ) {
    retour.push_back ( vec.at ( i ) );
  }
  return retour;
}
vector<float> copyVector ( vector<float> vec )
{
  vector<float> retour;
  for ( int i = 0; i < ( int ) vec.size(); i++ ) {
    retour.push_back ( vec.at ( i ) );
  }
  return retour;
}
vector<string> stringToVector ( string s, string tok )
{
  vector<string> to_return;
  string to_push ( "" );
  bool pushed = false;
  string::iterator sIt;
  for ( sIt = s.begin(); sIt < s.end(); sIt++ ) {
    pushed = false;
    for ( string::iterator sTok = tok.begin(); sTok < tok.end(); sTok++ ) {
      if ( ( *sIt ) == ( *sTok ) ) {
        to_return.push_back ( to_push );
        to_push = "";
        pushed = true;
      }
    }
    if ( !pushed ) {
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
  bool pushed = false;
  string::iterator sIt;
  for ( sIt = s.begin(); sIt < s.end(); sIt++ ) {
    pushed = false;
    for ( string::iterator sTok = tok.begin(); sTok < tok.end(); sTok++ ) {
      if ( ( *sIt ) == ( *sTok ) ) {
        if ( ( int ) to_push.length() > 0 ) {
          to_return.push_back ( atoi ( to_push.c_str() ) );
        }
        to_push = "";
        pushed = true;
      }
    }
    if ( !pushed ) {
      to_push.push_back ( ( *sIt ) );
    }
  }
  if ( ( int ) to_push.length() > 0 ) {
    to_return.push_back ( atoi ( to_push.c_str() ) );
  }
  return to_return;
}
vector<float> stringToVectorFloat ( string s, string tok )
{
  vector<float> to_return;
  string to_push ( "" );
  bool pushed = false;
  string::iterator sIt;
  for ( sIt = s.begin(); sIt < s.end(); sIt++ ) {
    pushed = false;
    for ( string::iterator sTok = tok.begin(); sTok < tok.end(); sTok++ ) {
      if ( ( *sIt ) == ( *sTok ) ) {
        if ( ( int ) to_push.length() > 0 ) {
          to_return.push_back ( atof ( to_push.c_str() ) );
        }
        to_push = "";
        pushed = true;
      }
    }
    if ( !pushed ) {
      to_push.push_back ( ( *sIt ) );
    }
  }
  if ( ( int ) to_push.length() > 0 ) {
    to_return.push_back ( atoi ( to_push.c_str() ) );
  }
  return to_return;
}

string lowerCase ( string str )
{
  for ( int i = 0; i < ( int ) str.size(); i++ ) {
    if ( ( str[i] >= 0x41 ) && ( str[i] <= 0x5A ) ) {
      str[i] = str[i] + 0x20;
    }
  }
  return str;
}
string removePunctTercom ( string str )
{
  string str_mod = str;
  sregex rex;
  string replace;


  rex = sregex::compile ( "^[ ]+" );
  replace = "";
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\"]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[,]" );
  replace = " ";
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "([^0-9])([\\.])([^0-9])" );
  replace = ( "$1 $3" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "([^0-9])([\\.])([^0-9])" );
  replace = ( "$1 $3" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "([^0-9])([\\.])([^0-9])" );
  replace = ( "$1 $3" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "([\\.]$)" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\?]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\;]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\:]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\!]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\(]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\)]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[ ]+" );
  replace = " ";
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[ ]+$" );
  replace = "";
  str_mod = regex_replace ( str_mod, rex, replace );

  return str_mod;
}
string removePunct ( string str )
{
  string str_mod = str;
  sregex rex;
  string replace;


  rex = sregex::compile ( "^[ ]+" );
  replace = "";
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\"]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[,]" );
  replace = " ";
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "([^0-9])([\\.])([^0-9])" );
  replace = ( "$1 $3" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "([^0-9])([\\.])([^0-9])" );
  replace = ( "$1 $3" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "([^0-9])([\\.])([^0-9])" );
  replace = ( "$1 $3" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "([\\.]$)" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\?]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\;]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\:]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\!]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\(]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\)]" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[ ]+" );
  replace = " ";
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[ ]+$" );
  replace = "";
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "^[ ]+" );
  replace = "";
  str_mod = regex_replace ( str_mod, rex, replace );

  return str_mod;
}
string tokenizePunct ( string str )
{
  string str_mod = str;
  sregex rex = sregex::compile ( "(([^0-9])([\\,])([^0-9]))" );
  string replace ( "$2 $3 $4" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(([^0-9])([\\.])([^0-9]))" );
  replace = ( "$2 $3 $4" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "( ([A-Z]|[a-z]) ([\\.]) )" );
  replace = ( " $2. " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "( ([A-Z]|[a-z]) ([\\.])$)" );
  replace = ( " $2. " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(^([A-Z]|[a-z]) ([\\.]) )" );
  replace = ( " $2. " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(([A-Z]|[a-z])([\\.]) ([A-Z]|[a-z])([\\.]) )" );
  replace = ( "$2.$4. " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\?]" );
  replace = ( " ? " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\;]" );
  replace = ( " ; " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(([^0-9])([\\:])([^0-9]))" );
  replace = ( "$2 $3 $4" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\!]" );
  replace = ( " ! " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\(]" );
  replace = ( " ( " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\\)]" );
  replace = ( " ) " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[\"]" );
  replace = ( " \" " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(num_ \\( ([^\\)]+) \\))" );
  replace = ( "num_($2)" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(ordinal_ \\( ([^\\)]*) \\))" );
  replace = ( "ordinal_($2)" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(^([Mm]) \\.)" );
  replace = ( "$2." );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "( ([Mm]) \\.)" );
  replace = ( " $2." );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(^([Dd]r) \\.)" );
  replace = ( "$2." );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "( ([Dd]r) \\.)" );
  replace = ( " $2." );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(^([Mm]r) \\.)" );
  replace = ( "$2." );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "( ([Mm]r) \\.)" );
  replace = ( " $2." );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(^([Mm]rs) \\.)" );
  replace = ( "$2." );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "( ([Mm]rs) \\.)" );
  replace = ( " $2." );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(^([Nn]o) \\.)" );
  replace = ( "$2." );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "( ([Nn]o) \\.)" );
  replace = ( " $2." );
  str_mod = regex_replace ( str_mod, rex, replace );

// 	rex = sregex::compile ( "(^(([Jj]an)|([Ff]ev)|([Mm]ar)|([Aa]pr)|([Jj]un)|([Jj]ul)|([Aa]ug)|([Ss]ept)|([Oo]ct)|([Nn]ov)|([Dd]ec)) \\.)" );
//         replace = ( "$2." );
//         str_mod = regex_replace ( str_mod, rex, replace );
//
//         rex = sregex::compile ( "( (([Jj]an)|([Ff]ev)|([Mm]ar)|([Aa]pr)|([Jj]un)|([Jj]ul)|([Aa]ug)|([Ss]ept)|([Oo]ct)|([Nn]ov)|([Dd]ec)) \\.)" );
//         replace = ( " $2." );
//         str_mod = regex_replace ( str_mod, rex, replace );
//
// 	rex = sregex::compile ( "(^(([Gg]en)|([Cc]ol)) \\.)" );
//         replace = ( "$2." );
//         str_mod = regex_replace ( str_mod, rex, replace );
//
//         rex = sregex::compile ( "( (([Gg]en)|([Cc]ol)) \\.)" );
//         replace = ( " $2." );
//         str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(^(([A-Z][a-z])) \\. )" );
  replace = ( "$2. " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "( (([A-Z][a-z])) \\. )" );
  replace = ( " $2. " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "(^(([A-Z][a-z][a-z])) \\. )" );
  replace = ( "$2. " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "( (([A-Z][a-z][a-z])) \\. )" );
  replace = ( " $2. " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[ ]+" );
  replace = " ";
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "^[ ]+" );
  replace = "";
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "[ ]+$" );
  replace = "";
  str_mod = regex_replace ( str_mod, rex, replace );

  return str_mod;
}

string normalizeStd ( string str )
{
  string str_mod = str;
  sregex rex = sregex::compile ( "(<skipped>)" );
  string replace ( "" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "-\n" );
  replace = ( "" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "\n" );
  replace = ( " " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "&quot;" );
  replace = ( "\"" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "&amp;" );
  replace = ( "& " );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "&lt;" );
  replace = ( "<" );
  str_mod = regex_replace ( str_mod, rex, replace );

  rex = sregex::compile ( "&gt;" );
  replace = ( ">" );
  str_mod = regex_replace ( str_mod, rex, replace );

  return str_mod;
}

param copyParam ( param p )
{
  param to_return;
  to_return.caseOn = p.caseOn;
  to_return.noPunct = p.noPunct;
  to_return.debugMode = p.debugMode;
  to_return.debugLevel = p.debugLevel;
  to_return.hypothesisFile = p.hypothesisFile;
  to_return.referenceFile = p.referenceFile;
  to_return.normalize = p.normalize;
  to_return.noTxtIds = p.noTxtIds;
  to_return.verbose = p.verbose;
  to_return.count_verbose = p.count_verbose;
  to_return.outputFileExtension = p.outputFileExtension;
  to_return.outputFileName = p.outputFileName;
  to_return.sgmlInputs = p.sgmlInputs;
  to_return.tercomLike = p.tercomLike;
  to_return.printAlignments = p.printAlignments;
  to_return.WER=p.WER;
  return to_return;
}
string printParams ( param p )
{
  stringstream s;
  s << "caseOn = " << p.caseOn << endl;
  s << "noPunct = " << p.noPunct << endl;
  s << "debugMode = " << p.debugMode << endl;
  s << "debugLevel = " << p.debugLevel << endl;
  s << "hypothesisFile = " << p.hypothesisFile << endl;
  s << "referenceFile = " << p.referenceFile << endl;
  s << "normalize = " << p.normalize << endl;
  s << "noTxtIds = " << p.noTxtIds << endl;
  s << "outputFileExtension = " << p.outputFileExtension << endl;
  s << "outputFileName = " << p.outputFileName << endl;
  s << "sgmlInputs = " << p.sgmlInputs << endl;
  s << "tercomLike = " << p.tercomLike << endl;
  s << "verbose = " << p.verbose << endl;
  s << "count_verbose = " << p.count_verbose << endl;
  return s.str();

}
string join ( string delim, vector<string> arr )
{
  if ( ( int ) arr.size() == 0 ) return "";
// 		if ((int)delim.compare("") == 0) delim = new String("");
// 		String s = new String("");
  stringstream s;
  s.str ( "" );
  for ( int i = 0; i < ( int ) arr.size(); i++ ) {
    if ( i == 0 ) {
      s << arr.at ( i );
    } else {
      s << delim << arr.at ( i );
    }
  }
  return s.str();
// 		return "";
}

}
