
#include "terAlignment.h"
using namespace std;
namespace TERCpp
{

terAlignment::terAlignment()
{
// 		vector<string> ref;
// 		vector<string> hyp;
// 		vector<string> aftershift;

  //   TERshift[] allshifts = null;

  numEdits=0;
  numWords=0;
  bestRef="";

  numIns=0;
  numDel=0;
  numSub=0;
  numSft=0;
  numWsf=0;
}
string terAlignment::toString()
{
  stringstream s;
  s.str ( "" );
  s << "Original Ref: " << join ( " ", ref ) << endl;
  s << "Original Hyp: " << join ( " ", hyp ) <<endl;
  s << "Hyp After Shift: " << join ( " ", aftershift );
  s << endl;
// 		string s = "Original Ref: " + join(" ", ref) + 	"\nOriginal Hyp: " + join(" ", hyp) + "\nHyp After Shift: " + join(" ", aftershift);
  if ( ( int ) sizeof ( alignment ) >0 ) {
    s << "Alignment: (";
// 			s += "\nAlignment: (";
    for ( int i = 0; i < ( int ) ( alignment.size() ); i++ ) {
      s << alignment[i];
// 				s+=alignment[i];
    }
// 			s += ")";
    s << ")";
  }
  s << endl;
  if ( ( int ) allshifts.size() == 0 ) {
// 			s += "\nNumShifts: 0";
    s << "NumShifts: 0";
  } else {
// 			s += "\nNumShifts: " + (int)allshifts.size();
    s << "NumShifts: "<< ( int ) allshifts.size();
    for ( int i = 0; i < ( int ) allshifts.size(); i++ ) {
      s << endl << "  " ;
      s << ( ( terShift ) allshifts[i] ).toString();
// 				s += "\n  " + allshifts[i];
    }
  }
  s << endl << "Score: " << scoreAv() << " (" << numEdits << "/" << averageWords << ")";
// 		s += "\nScore: " + score() + " (" + numEdits + "/" + numWords + ")";
  return s.str();

}
string terAlignment::join ( string delim, vector<string> arr )
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
double terAlignment::score()
{
  if ( ( numWords <= 0.0 ) && ( numEdits > 0.0 ) ) {
    return 1.0;
  }
  if ( numWords <= 0.0 ) {
    return 0.0;
  }
  return ( double ) numEdits / numWords;
}
double terAlignment::scoreAv()
{
  if ( ( averageWords <= 0.0 ) && ( numEdits > 0.0 ) ) {
    return 1.0;
  }
  if ( averageWords <= 0.0 ) {
    return 0.0;
  }
  return ( double ) numEdits / averageWords;
}

void terAlignment::scoreDetails()
{
  numIns = numDel = numSub = numWsf = numSft = 0;
  if((int)allshifts.size()>0) {
    for(int i = 0; i < (int)allshifts.size(); ++i) {
      numWsf += allshifts[i].size();
    }
    numSft = allshifts.size();
  }

  if((int)alignment.size()>0 ) {
    for(int i = 0; i < (int)alignment.size(); ++i) {
      switch (alignment[i]) {
      case 'S':
      case 'T':
        numSub++;
        break;
      case 'D':
        numDel++;
        break;
      case 'I':
        numIns++;
        break;
      }
    }
  }
  //	if(numEdits != numSft + numDel + numIns + numSub)
  //      System.out.println("** Error, unmatch edit erros " + numEdits +
  //                         " vs " + (numSft + numDel + numIns + numSub));
}

}