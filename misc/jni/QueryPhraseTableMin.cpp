// Query binary phrase tables.
// Marcin Junczys-Dowmunt, 13 September 2012

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "QueryPhraseTableMin.h"
#include "CompactPT/PhraseDictionaryCompact.h"
#include "LMList.h"
#include "Util.h"

using namespace Moses;

QueryPhraseTableMin::QueryPhraseTableMin(const std::string& ttable)
{
  m_nscores = 5;
  m_useAlignments = true;
  m_reportCounts = false;

  //for(int i = 1; i < argc; i++) {
  //  if(!strcmp(argv[i], "-n")) {
  //    if(i + 1 == argc)
  //      usage();
  //    nscores = atoi(argv[++i]);
  //  } else if(!strcmp(argv[i], "-s")) {
  //    if(i + 1 == argc)
  //      usage();
  //    paramSource = argv[++i];
  //    single = true;
  //  } else if(!strcmp(argv[i], "-t")) {
  //    if(i + 1 == argc)
  //      usage();
  //    ttable = argv[++i];
  //  } else if(!strcmp(argv[i], "-a")) {
  //    useAlignments = true;
  //  } else if (!strcmp(argv[i], "-c")) {
  //    reportCounts = true;
  //  }
  //  else
  //    usage();
  //}
  //
  //if(ttable == "")
  //  usage();

  m_input = std::vector<FactorType>(1, 0);
  m_output = std::vector<FactorType>(1, 0);
  m_weight = std::vector<float>(m_nscores, 0);
  
  m_lmList.reset(new LMList());
  
  Parameter *parameter = new Parameter();
  const_cast<std::vector<std::string>&>(parameter->GetParam("factor-delimiter")).resize(1, "||dummy_string||");
  const_cast<std::vector<std::string>&>(parameter->GetParam("input-factors")).resize(1, "0");
  const_cast<std::vector<std::string>&>(parameter->GetParam("verbose")).resize(1, "0");
  const_cast<std::vector<std::string>&>(parameter->GetParam("weight-w")).resize(1, "0");
  const_cast<std::vector<std::string>&>(parameter->GetParam("weight-d")).resize(1, "0");
  
  const_cast<StaticData&>(StaticData::Instance()).LoadData(parameter);

  m_pdf.reset(new PhraseDictionaryFeature(Compact, m_nscores, m_nscores, m_input, m_output, ttable, m_weight, 0, "", ""));
  m_pdc.reset(new PhraseDictionaryCompact(m_nscores, Compact, m_pdf.get(), false, m_useAlignments));
  
  bool ret = m_pdc->Load(m_input, m_output, ttable, m_weight, 0, *m_lmList, 0);                                                                           
  assert(ret);
}

std::string QueryPhraseTableMin::query(std::string phrase)
{
    
  std::string sourceString = phrase;
  
  Phrase sourcePhrase(0);
  sourcePhrase.CreateFromString(m_input, sourceString, "||dummy_string||");
  
  TargetPhraseVectorPtr decodedPhraseColl
    = m_pdc->GetTargetPhraseCollectionRaw(sourcePhrase);
  
  std::stringstream output;
  if(decodedPhraseColl != NULL) {
    if(m_reportCounts)
      output << sourcePhrase << decodedPhraseColl->size() << std::endl;
    else
      for(TargetPhraseVector::iterator it = decodedPhraseColl->begin(); it != decodedPhraseColl->end(); it++) {
        TargetPhrase &tp = *it;
        output << sourcePhrase << "||| ";
        output << static_cast<const Phrase&>(tp) << "|||";
        
        if(m_useAlignments)
          output << " " << tp.GetAlignmentInfo() << "|||"; 
        
        size_t offset = tp.GetScoreBreakdown().size() - m_nscores;
        for(size_t i = offset; i < tp.GetScoreBreakdown().size(); i++)
          output << " " << exp(tp.GetScoreBreakdown()[i]);
        output << std::endl;
      }
  }
  return output.str();
}

void QueryPhraseTableMin::usage()
{
  std::cerr << 	"Usage: queryPhraseTable [-n <nscores>] [-a] -t <ttable>\n"
            "-n <nscores>      number of scores in phrase table (default: 5)\n"
            "-c                only report counts of entries\n"
            "-a                binary phrase table contains alignments\n"
            "-t <ttable>       phrase table\n"
            "-s <string>       query single source phrase string\n";
  exit(1);
}
