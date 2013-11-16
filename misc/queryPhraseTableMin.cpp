// Query binary phrase tables.
// Marcin Junczys-Dowmunt, 13 September 2012

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "moses/TranslationModel/CompactPT/PhraseDictionaryCompact.h"
#include "moses/Util.h"
#include "moses/Phrase.h"

void usage();

typedef unsigned int uint;

using namespace Moses;

int main(int argc, char **argv)
{
  int nscores = 5;
  std::string ttable = "";
  bool useAlignments = false;
  bool reportCounts = false;

  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-n")) {
      if(i + 1 == argc)
        usage();
      nscores = atoi(argv[++i]);
    } else if(!strcmp(argv[i], "-t")) {
      if(i + 1 == argc)
        usage();
      ttable = argv[++i];
    } else if(!strcmp(argv[i], "-a")) {
      useAlignments = true;
    } else if (!strcmp(argv[i], "-c")) {
      reportCounts = true;
    } else
      usage();
  }

  if(ttable == "")
    usage();

  std::vector<FactorType> input(1, 0);
  std::vector<FactorType> output(1, 0);
  std::vector<float> weight(nscores, 0);

  Parameter *parameter = new Parameter();
  const_cast<std::vector<std::string>&>(parameter->GetParam("factor-delimiter")).resize(1, "||dummy_string||");
  const_cast<std::vector<std::string>&>(parameter->GetParam("input-factors")).resize(1, "0");
  const_cast<std::vector<std::string>&>(parameter->GetParam("verbose")).resize(1, "0");
  //const_cast<std::vector<std::string>&>(parameter->GetParam("weight-w")).resize(1, "0");
  //const_cast<std::vector<std::string>&>(parameter->GetParam("weight-d")).resize(1, "0");

  StaticData::InstanceNonConst().LoadData(parameter);

  PhraseDictionaryCompact pdc("PhraseDictionaryCompact input-factor=0 output-factor=0 num-features=5 path=" + ttable);
  pdc.Load();

  std::string line;
  while(getline(std::cin, line)) {
    Phrase sourcePhrase;
    sourcePhrase.CreateFromString(Input, input, line, "||dummy_string||", NULL);

    TargetPhraseVectorPtr decodedPhraseColl
    = pdc.GetTargetPhraseCollectionRaw(sourcePhrase);

    if(decodedPhraseColl != NULL) {
      if(reportCounts)
        std::cout << sourcePhrase << decodedPhraseColl->size() << std::endl;
      else
        for(TargetPhraseVector::iterator it = decodedPhraseColl->begin(); it != decodedPhraseColl->end(); it++) {
          TargetPhrase &tp = *it;
          std::cout << sourcePhrase << "||| ";
          std::cout << static_cast<const Phrase&>(tp) << "|||";

          if(useAlignments)
            std::cout << " " << tp.GetAlignTerm() << "|||";

          std::vector<float> scores = tp.GetScoreBreakdown().GetScoresForProducer(&pdc);
          for(size_t i = 0; i < scores.size(); i++)
            std::cout << " " << exp(scores[i]);
          std::cout << std::endl;
        }
    } else if(reportCounts)
      std::cout << sourcePhrase << 0 << std::endl;

    std::cout.flush();
  }
}

void usage()
{
  std::cerr << 	"Usage: queryPhraseTable [-n <nscores>] [-a] -t <ttable>\n"
            "-n <nscores>      number of scores in phrase table (default: 5)\n"
            "-c                only report counts of entries\n"
            "-a                binary phrase table contains alignments\n"
            "-t <ttable>       phrase table\n";
  exit(1);
}
