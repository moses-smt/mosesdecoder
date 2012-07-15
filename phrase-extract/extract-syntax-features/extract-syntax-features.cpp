#include "XmlTree.h"
#include "InputFileStream.h"
#include "InputTreeRep.h"
#include "TypeDef.h"

#define LINE_MAX_LENGTH 500000

using namespace std;

int main(int argc, char* argv[])
{
    char* &fileNameSource = argv[1];
    string fileNameExtract = string(argv[2]);

    cerr << "extracting syntactic features" << endl;

    // open input files
    Moses::InputFileStream sourceFile(fileNameSource);
    //Moses::InputFileStream sFile(fileNameS);
    //Moses::InputFileStream aFile(fileNameA);

    istream *sourceFileP = &sourceFile;

    //Read in source sentence and create label chart
    Moses::InputTreeRep myInputChart = Moses::InputTreeRep();
    myInputChart.Read(sourceFile);
    myInputChart.Print(std::cout);

    //Read in extract file line by line
    /*sifstream in(&fileNameRuleTable);
    if (!in.good())
    return false;
    string line;
    while (getline(in, line)) {
    vector<string> columns = Tokenize(line, "\t");

    size_t sentenceId = Scan<size_t>(columns[0]);
    size_t startSource = Scan<size_t>(columns[1]);
    size_t endSource = Scan<size_t>(columns[2]);
    size_t startTarget = Scan<size_t>(columns[3]);
    size_t endTarget = Scan<size_t>(columns[4]);*/
}

