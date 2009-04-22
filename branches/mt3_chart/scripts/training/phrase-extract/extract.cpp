// $Id$

#include <cstdio>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <cstring>
#include <sstream>
#include "extract.h"

#ifdef WIN32
// Include Visual Leak Detector
//#include <vld.h>
#endif

using namespace std;

int main(int argc, char* argv[]) 
{
  cerr << "PhraseExtract v1.4, written by Philipp Koehn\n"
       << "phrase extraction from an aligned parallel corpus\n";
  time_t starttime = time(NULL);

  if (argc < 6) {
    cerr << "syntax: phrase-extract en de align extract max-length [maxNonTerm nonTermFirstWord nonTermConsec]"
					<< " [orientation | --OnlyOutputSpanInfo | --NoFileLimit | --ProperConditioning ]\n";
    exit(1);
  }
  char* &fileNameE = argv[1];
  char* &fileNameF = argv[2];
  char* &fileNameA = argv[3];
  fileNameExtract = argv[4];
  maxPhraseLength = atoi(argv[5]);

	int optionInd = 6;
	if (argc > 6 && (string(argv[6]).size() < 2 || string(argv[6]).substr(2) != "--"))
	{ // hiero extraction
		maxNonTerm = atoi(argv[6]);
		nonTermFirstWord = atoi(argv[7]);
		nonTermConsec = atoi(argv[8]);

		optionInd = 9;
	}

  for(int i=optionInd;i<argc;i++) {
    if (strcmp(argv[i],"--OnlyOutputSpanInfo") == 0) {
      onlyOutputSpanInfo = true;
    }
    else if (strcmp(argv[i],"--NoFileLimit") == 0) {
      noFileLimit = true;
    }
    else if (strcmp(argv[i],"orientation") == 0 || strcmp(argv[i],"--Orientation") == 0) {
      orientationFlag = true;
    }
    else if (strcmp(argv[i],"--ZipFiles") == 0) {
      zipFiles = true;
    }
    else if (strcmp(argv[i],"--ProperConditioning") == 0) {
      properConditioning = true;
    }
    else {
      cerr << "extract: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }
  ifstream eFile;
  ifstream fFile;
  ifstream aFile;
  eFile.open(fileNameE);
  fFile.open(fileNameF);
  aFile.open(fileNameA);
  istream *eFileP = &eFile;
  istream *fFileP = &fFile;
  istream *aFileP = &aFile;
  
  int i=0;
  while(true) {
    i++;
    if (i%10000 == 0) cerr << "." << flush;
    char englishString[LINE_MAX_LENGTH];
    char foreignString[LINE_MAX_LENGTH];
    char alignmentString[LINE_MAX_LENGTH];
    SAFE_GETLINE((*eFileP), englishString, LINE_MAX_LENGTH, '\n');
    if (eFileP->eof()) break;
    SAFE_GETLINE((*fFileP), foreignString, LINE_MAX_LENGTH, '\n');
    SAFE_GETLINE((*aFileP), alignmentString, LINE_MAX_LENGTH, '\n');
    SentenceAlignment sentence;
    // cout << "read in: " << englishString << " & " << foreignString << " & " << alignmentString << endl;
    //az: output src, tgt, and alingment line
    if (onlyOutputSpanInfo) {
      cout << "LOG: SRC: " << foreignString << endl;
      cout << "LOG: TGT: " << englishString << endl;
      cout << "LOG: ALT: " << alignmentString << endl;
      cout << "LOG: PHRASES_BEGIN:" << endl;
    }
      
    if (sentence.create( englishString, foreignString, alignmentString, i )) {
      extract(sentence);
      if (properConditioning) extractBase(sentence);
    }
    if (onlyOutputSpanInfo) cout << "LOG: PHRASES_END:" << endl; //az: mark end of phrases
  }

  eFile.close();
  fFile.close();
  aFile.close();
  //az: only close if we actually opened it
  if (!onlyOutputSpanInfo) {
    extractFile.close();
    extractFileInv.close();
    if (orientationFlag) extractFileOrientation.close();
  }
}
 
// if proper conditioning, we need the number of times a foreign phrase occured
void extractBase( SentenceAlignment &sentence ) {
  int countF = sentence.foreign.size();
  for(int startF=0;startF<countF;startF++) {
    for(int endF=startF;
        (endF<countF && endF<startF+maxPhraseLength);
        endF++) {
      for(int fi=startF;fi<=endF;fi++) {
	extractFile << sentence.foreign[fi] << " ";
      }
      extractFile << "|||" << endl;
    }
  }

  int countE = sentence.english.size();
  for(int startE=0;startE<countE;startE++) {
    for(int endE=startE;
        (endE<countE && endE<startE+maxPhraseLength);
        endE++) {
      for(int ei=startE;ei<=endE;ei++) {
	extractFileInv << sentence.english[ei] << " ";
      }
      extractFileInv << "|||" << endl;
    }
  }
}

void extract( SentenceAlignment &sentence ) {
	int countE = sentence.english.size();
	int countF = sentence.foreign.size();

	// for creating hiero phrases
	PhraseExist phraseExist(countF);

	// check alignments for english phrase startE...endE
	for(int startE=0;startE<countE;startE++) {
		for(int endE=startE;
				(endE<countE && endE<startE+maxPhraseLength);
				endE++) {
			
			int minF = 9999;
			int maxF = -1;
			vector< int > usedF = sentence.alignedCountF;
			for(int ei=startE;ei<=endE;ei++) {
				for(int i=0;i<sentence.alignedToE[ei].size();i++) {
					int fi = sentence.alignedToE[ei][i];
					// cout << "point (" << fi << ", " << ei << ")\n";
					if (fi<minF) { minF = fi; }
					if (fi>maxF) { maxF = fi; }
					usedF[ fi ]--;
				}
			}
			
			// cout << "f projected ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")\n"; 

			if (maxF >= 0 && // aligned to any foreign words at all
					maxF-minF < maxPhraseLength) { // foreign phrase within limits
				
				// check if foreign words are aligned to out of bound english words
				bool out_of_bounds = false;
				for(int fi=minF;fi<=maxF && !out_of_bounds;fi++)
				if (usedF[fi]>0) {
					// cout << "ouf of bounds: " << fi << "\n";
					out_of_bounds = true;
				}
				
				// cout << "doing if for ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")\n"; 
				if (!out_of_bounds)
				{
					// start point of foreign phrase may retreat over unaligned
					for(int startF=minF;
							(startF>=0 &&
								startF>maxF-maxPhraseLength && // within length limit
								(startF==minF || sentence.alignedCountF[startF]==0)); // unaligned
							startF--)
					{
						// end point of foreign phrase may advance over unaligned
						for(int endF=maxF;
								(endF<countF && endF<startF+maxPhraseLength && // within length limit
								(endF==maxF || sentence.alignedCountF[endF]==0)); // unaligned
								endF++) 
						{
							addPhrase(sentence,startE,endE,startF,endF, phraseExist);
							phraseExist.Add(startE, endE, startF, endF);
						}
					}
				} // if (!out_of_bounds)
			}
		}
	}

	// hiero phrase
	for(int startF=0;startF<countF;startF++) 
	{
		for(int endF=startF; endF<countF; endF++) 
		{
			const HoleList &targetHoles = phraseExist.GetTargetHoles(startF, endF);
			
			HoleList::const_iterator iter;
			for (iter = targetHoles.begin(); iter != targetHoles.end(); ++iter)
			{
				const Hole &targetHole = *iter;
				HoleCollection holeColl;
				int initStartF = nonTermFirstWord ? startF : startF + 1;

				addHieroPhrase(sentence, targetHole.GetStart(), targetHole.GetEnd()
											, startF, endF, phraseExist, holeColl, 0, initStartF);
			}
		}
	}
}

void printSourceHieroPhrase(SentenceAlignment &sentence, int startE, int endE, int startF, int endF 
							 , HoleCollection &holeColl, ostream &stream)
{
	HoleList::iterator iterHoleList = holeColl.GetSourceHoles().begin();
	assert(iterHoleList != holeColl.GetSourceHoles().end());

	int outPos = 0;
  for(int currPos = startF; currPos <= endF; currPos++) 
	{
		bool isHole = false;
		if (iterHoleList != holeColl.GetSourceHoles().end())
		{
			const Hole &hole = *iterHoleList;
			isHole = hole.GetStart() == currPos;
		}

		if (isHole)
		{
			stream << "[X] ";

			Hole &hole = *iterHoleList;
			currPos = hole.GetEnd();
			hole.SetPos(outPos);

			++iterHoleList;
		}
		else
		{
			stream << sentence.foreign[currPos] << " ";
		}

		outPos++;
	}

	assert(iterHoleList == holeColl.GetSourceHoles().end());
}


void printTargetHieroPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF 
							 , HoleCollection &holeColl, ostream &stream)
{
	vector<Hole*>::iterator iterHoleList = holeColl.GetSortedTargetHoles().begin();
	assert(iterHoleList != holeColl.GetSortedTargetHoles().end());

	int outPos = 0;
  for(int currPos = startE; currPos <= endE; currPos++) 
	{
		bool isHole = false;
		if (iterHoleList != holeColl.GetSortedTargetHoles().end())
		{
			const Hole &hole = **iterHoleList;
			isHole = hole.GetStart() == currPos;
		}

		if (isHole)
		{
			stream << "[X] ";

			Hole &hole = **iterHoleList;
			currPos = hole.GetEnd();
			hole.SetPos(outPos);

			++iterHoleList;
		}
		else
		{
			stream << sentence.english[currPos] << " ";
		}

		outPos++;
	}

	assert(iterHoleList == holeColl.GetSortedTargetHoles().end());
}

void printAlignment(HoleCollection &holeColl)
{
	HoleList::const_iterator iterSource, iterTarget;
	iterTarget = holeColl.GetTargetHoles().begin();
	for (iterSource = holeColl.GetSourceHoles().begin(); iterSource != holeColl.GetSourceHoles().end(); ++iterSource)
	{
		extractFile << iterSource->GetPos() << "-" << iterTarget->GetPos() << " ";
		extractFileInv << iterTarget->GetPos() << "-" << iterSource->GetPos() << " ";
		++iterTarget;
	}

	assert(iterTarget == holeColl.GetTargetHoles().end());
}

void printHieroPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF 
							 , HoleCollection &holeColl)
{
	openFiles();
	phraseCount++;

	stringstream sourceStream, targetStream;

	// source
	printSourceHieroPhrase(sentence, startE, endE, startF, endF, holeColl, sourceStream);
	
	// target
	holeColl.SortTargetHoles();
	printTargetHieroPhrase(sentence, startE, endE, startF, endF, holeColl, targetStream);

	// output to file
	extractFile << sourceStream.str() << "||| " << targetStream.str() << "||| ";
	extractFileInv << targetStream.str() << "||| " << sourceStream.str() << "||| ";

	// alignment
	printAlignment(holeColl);

	extractFile << endl;
	extractFileInv << endl;
}

void addHieroPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF 
							 , PhraseExist &phraseExist, const HoleCollection &holeColl, int numHoles, int initStartF)
{
	if (numHoles >= maxNonTerm)
		return;

	for (int startHoleF = initStartF; startHoleF <= endF; ++startHoleF)
	{
		for (int endHoleF = startHoleF; endHoleF <= endF; ++endHoleF)
		{
			// except the whole span
			if (startHoleF == startF && endHoleF == endF)
				continue;

			// does a phrase cover this source span. if it does, then there should be corresponding target span
			// a list of them actually, just in case phrase heuristic is slack
			const HoleList &targetHoles = phraseExist.GetTargetHoles(startHoleF, endHoleF);

			// search for hole
			HoleList::const_iterator iterTargetHoles;
			for (iterTargetHoles = targetHoles.begin(); iterTargetHoles != targetHoles.end(); ++iterTargetHoles)
			{
				const Hole &targetHole = *iterTargetHoles;

				// hole must be subphrase of the phrase
				if (startE > targetHole.GetStart() || endE <  targetHole.GetEnd())
					continue;

				// make sure target side doesn't overlap with another hole
				if (holeColl.OverlapTarget(targetHole))
					continue;

				HoleCollection copyHoleColl(holeColl);
				copyHoleColl.Add(targetHole.GetStart(), targetHole.GetEnd(), startHoleF, endHoleF);

				// print out this 1
				printHieroPhrase(sentence, startE, endE, startF, endF, copyHoleColl);

				// recursively search for next hole
				int nextInitStartF = nonTermConsec ? endHoleF + 1 : endHoleF + 2;
				addHieroPhrase(sentence, startE, endE, startF, endF 
											, phraseExist, copyHoleColl, numHoles + 1, nextInitStartF);

				// loop continue to extend search for another hole in same phrase
			}
		}
	}
}

void openFiles()
{
  // new file every 1e7 phrases
  if (phraseCount % 10000000 == 0 // new file every 1e7 phrases
      && (!noFileLimit || phraseCount == 0)) { // only new partial file, if file limit

    // close old file
    if (!noFileLimit && phraseCount>0) {
      extractFile.close();
      extractFileInv.close();
      if (orientationFlag) extractFileOrientation.close();
    }
    
    // construct file name
    char part[10];
    if (noFileLimit)
      part[0] = '\0';
    else
      sprintf(part,".part%04d",phraseCount/10000000);  
    string fileNameExtractPart = string(fileNameExtract) + part;
    string fileNameExtractInvPart = string(fileNameExtract) + ".inv" + part;
    string fileNameExtractOrientationPart = string(fileNameExtract) + ".o" + part;

    
    // open files
    extractFile.open(fileNameExtractPart.c_str());
    extractFileInv.open(fileNameExtractInvPart.c_str());
    if (orientationFlag) 
      extractFileOrientation.open(fileNameExtractOrientationPart.c_str());
  }
}

void addPhrase( SentenceAlignment &sentence, int startE, int endE, int startF, int endF 
							 , PhraseExist &phraseExist) 
{
  // foreign
  // cout << "adding ( " << startF << "-" << endF << ", " << startE << "-" << endE << ")\n"; 

  if (onlyOutputSpanInfo) {
    cout << startF << " " << endF << " " << startE << " " << endE << endl;
    return;
  } 
	openFiles();

  phraseCount++;

  for(int fi=startF;fi<=endF;fi++) {
    extractFile << sentence.foreign[fi] << " ";
    if (orientationFlag) extractFileOrientation << sentence.foreign[fi] << " ";
  }
  extractFile << "||| ";
  if (orientationFlag) extractFileOrientation << "||| ";

  // english
  for(int ei=startE;ei<=endE;ei++) {
    extractFile << sentence.english[ei] << " ";
    extractFileInv << sentence.english[ei] << " ";
    if (orientationFlag) extractFileOrientation << sentence.english[ei] << " ";
  }
  extractFile << "|||";
  extractFileInv << "||| ";
  if (orientationFlag) extractFileOrientation << "||| ";

  // foreign (for inverse)
  for(int fi=startF;fi<=endF;fi++)
    extractFileInv << sentence.foreign[fi] << " ";
  extractFileInv << "|||";

  // alignment
  for(int ei=startE;ei<=endE;ei++) 
    for(int i=0;i<sentence.alignedToE[ei].size();i++) {
      int fi = sentence.alignedToE[ei][i];
      extractFile << " " << fi-startF << "-" << ei-startE;
      extractFileInv << " " << ei-startE << "-" << fi-startF;
    }

  if (orientationFlag) {

    // orientation to previous E
    bool connectedLeftTop  = isAligned( sentence, startF-1, startE-1 );
    bool connectedRightTop = isAligned( sentence, endF+1,   startE-1 );
    if      ( connectedLeftTop && !connectedRightTop) 
      extractFileOrientation << "mono";
    else if (!connectedLeftTop &&  connectedRightTop) 
      extractFileOrientation << "swap";
    else 
      extractFileOrientation << "other";
  
    // orientation to following E
    bool connectedLeftBottom  = isAligned( sentence, startF-1, endE+1 );
    bool connectedRightBottom = isAligned( sentence, endF+1,   endE+1 );
    if      ( connectedLeftBottom && !connectedRightBottom) 
      extractFileOrientation << " swap";
    else if (!connectedLeftBottom &&  connectedRightBottom) 
      extractFileOrientation << " mono";
    else 
      extractFileOrientation << " other";
  }

  extractFile << "\n";
  extractFileInv << "\n";
  if (orientationFlag) extractFileOrientation << "\n";
}
  
bool isAligned ( SentenceAlignment &sentence, int fi, int ei ) {
  if (ei == -1 && fi == -1) return true;
  if (ei <= -1 || fi <= -1) return false;
  if (ei == sentence.english.size() && fi == sentence.foreign.size()) return true;
  if (ei >= sentence.english.size() || fi >= sentence.foreign.size()) return false;
  for(int i=0;i<sentence.alignedToE[ei].size();i++) 
    if (sentence.alignedToE[ei][i] == fi) return true;
  return false;
}


int SentenceAlignment::create( char englishString[], char foreignString[], char alignmentString[], int sentenceID ) {
  english = tokenize( englishString );
  foreign = tokenize( foreignString );
  //  alignment = new bool[foreign.size()*english.size()];
  //  alignment = (bool**) calloc(english.size()*foreign.size(),sizeof(bool)); // is this right?
  
  if (english.size() == 0 || foreign.size() == 0) {
    cerr << "no english (" << english.size() << ") or foreign (" << foreign.size() << ") words << end insentence " << sentenceID << endl;
    cerr << "E: " << englishString << endl << "F: " << foreignString << endl;
    return 0;
  }
  // cout << "english.size = " << english.size() << endl;
  // cout << "foreign.size = " << foreign.size() << endl;

  // cout << "xxx\n";
  for(int i=0; i<foreign.size(); i++) {
    // cout << "i" << i << endl;
    alignedCountF.push_back( 0 );
  }
  for(int i=0; i<english.size(); i++) {
    vector< int > dummy;
    alignedToE.push_back( dummy );
  }
  // cout << "\nscanning...\n";

  vector<string> alignmentSequence = tokenize( alignmentString );
  for(int i=0; i<alignmentSequence.size(); i++) {
    int e,f;
    // cout << "scaning " << alignmentSequence[i].c_str() << endl;
    if (! sscanf(alignmentSequence[i].c_str(), "%d-%d", &f, &e)) {
      cerr << "WARNING: " << alignmentSequence[i] << " is a bad alignment point in sentnce " << sentenceID << endl; 
      cerr << "E: " << englishString << endl << "F: " << foreignString << endl;
      return 0;
    }
      // cout << "alignmentSequence[i] " << alignmentSequence[i] << " is " << f << ", " << e << endl;
    if (e >= english.size() || f >= foreign.size()) { 
      cerr << "WARNING: sentence " << sentenceID << " has alignment point (" << f << ", " << e << ") out of bounds (" << foreign.size() << ", " << english.size() << ")\n";
      cerr << "E: " << englishString << endl << "F: " << foreignString << endl;
      return 0;
    }
    alignedToE[e].push_back( f );
    alignedCountF[f]++;
  }
  return 1;
}

