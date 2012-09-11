#include "SuffixArray.h"

using namespace std;

int main(int argc, char* argv[]) 
{
	SuffixArray suffixArray( "/home/pkoehn/syntax/grammars/wmt09-de-en/corpus.1k.de" );
	//suffixArray.List(10,20);
	vector< string > der;
	der.push_back("der");
	vector< string > inDer;
	inDer.push_back("in");
	inDer.push_back("der");
	vector< string > zzz;
	zzz.push_back("zzz");
	vector< string > derDer;
	derDer.push_back("der");
	derDer.push_back("der");

	cout << "count of 'der' " << suffixArray.Count( der ) << endl;
	cout << "limited count of 'der' " << suffixArray.MinCount( der, 2 ) << endl;
	cout << "count of 'in der' " << suffixArray.Count( inDer ) << endl;
	cout << "count of 'der der' " << suffixArray.Count( derDer ) << endl;
	cout << "limited count of 'der der' " << suffixArray.MinCount( derDer, 1 ) << endl;
	// cout << "count of 'zzz' " << suffixArray.Count( zzz ) << endl;
	// cout << "limited count of 'zzz' " << suffixArray.LimitedCount( zzz, 1 ) << endl;
}
