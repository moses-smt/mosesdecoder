// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
// $Id$

#include "ConfusionNet.h"
#include <sstream>

#include "FactorCollection.h"
#include "Util.h"
#include "TranslationOptionCollectionConfusionNet.h"
#include "StaticData.h"
#include "Sentence.h"
#include "moses/FF/InputFeature.h"
#include "util/exception.hh"
#include "moses/TranslationTask.h"
namespace Moses
{
struct CNStats {
  size_t created,destr,read,colls,words;

  CNStats() : created(0),destr(0),read(0),colls(0),words(0) {}
  ~CNStats() {
    print(std::cerr);
  }

  void createOne() {
    ++created;
  }
  void destroyOne() {
    ++destr;
  }

  void collect(const ConfusionNet& cn) {
    ++read;
    colls+=cn.GetSize();
    for(size_t i=0; i<cn.GetSize(); ++i)
      words+=cn[i].size();
  }
  void print(std::ostream& out) const {
    if(created>0) {
      out<<"confusion net statistics:\n"
         " created:\t"<<created<<"\n"
         " destroyed:\t"<<destr<<"\n"
         " succ. read:\t"<<read<<"\n"
         " columns:\t"<<colls<<"\n"
         " words:\t"<<words<<"\n"
         " avg. word/column:\t"<<words/(1.0*colls)<<"\n"
         " avg. cols/sent:\t"<<colls/(1.0*read)<<"\n"
         "\n\n";
    }
  }
};

CNStats stats;

size_t
ConfusionNet::
GetColumnIncrement(size_t i, size_t j) const
{
  (void) i;
  (void) j;
  return 1;
}

ConfusionNet::
ConfusionNet(AllOptions::ptr const& opts) : InputType(opts)
{
  stats.createOne();

  if (is_syntax(opts->search.algo)) {
    m_defaultLabelSet.insert(opts->syntax.input_default_non_terminal);
  }
  UTIL_THROW_IF2(InputFeature::InstancePtr() == NULL, "Input feature must be specified");
}

ConfusionNet::
~ConfusionNet()
{
  stats.destroyOne();
}

ConfusionNet::
ConfusionNet(Sentence const& s) : InputType(s.options())
{
  data.resize(s.GetSize());
  for(size_t i=0; i<s.GetSize(); ++i) {
    ScorePair scorePair;
    std::pair<Word, ScorePair > temp = std::make_pair(s.GetWord(i), scorePair);
    data[i].push_back(temp);
  }
}

bool
ConfusionNet::
ReadF(std::istream& in, int format)
{
  VERBOSE(2, "read confusion net with format "<<format<<"\n");
  switch(format) {
  case 0:
    return ReadFormat0(in);
  case 1:
    return ReadFormat1(in);
  default:
    std::cerr << "ERROR: unknown format '"<<format
              <<"' in ConfusionNet::Read";
  }
  return false;
}

int
ConfusionNet::
Read(std::istream& in)
{
  int rv=ReadF(in,0);
  if(rv) stats.collect(*this);
  return rv;
}

bool
ConfusionNet::
ReadFormat0(std::istream& in)
{
  Clear();
  const std::vector<FactorType>& factorOrder = m_options->input.factor_order;

  const InputFeature *inputFeature = InputFeature::InstancePtr();
  size_t numInputScores   = inputFeature->GetNumInputScores();
  size_t numRealWordCount = inputFeature->GetNumRealWordsInInput();

  size_t totalCount = numInputScores + numRealWordCount;
  bool addRealWordCount = (numRealWordCount > 0);

  std::string line;
  while(getline(in,line)) {
    std::istringstream is(line);
    std::string word;

    Column col;
    while(is>>word) {
      Word w;
      w.CreateFromString(Input,factorOrder,StringPiece(word),false,false);
      std::vector<float> probs(totalCount, 0.0);
      for(size_t i=0; i < numInputScores; i++) {
        double prob;
        if (!(is>>prob)) {
          TRACE_ERR("ERROR: unable to parse CN input - bad link probability, "
                    << "or wrong number of scores\n");
          return false;
        }
        if(prob<0.0) {
          VERBOSE(1, "WARN: negative prob: "<<prob<<" ->set to 0.0\n");
          prob=0.0;
        } else if (prob>1.0) {
          VERBOSE(1, "WARN: prob > 1.0 : "<<prob<<" -> set to 1.0\n");
          prob=1.0;
        }
        probs[i] = (std::max(static_cast<float>(log(prob)),LOWEST_SCORE));

      }
      // store 'real' word count in last feature if we have one more
      // weight than we do arc scores and not epsilon
      if (addRealWordCount && word!=EPSILON && word!="")
        probs.back() = -1.0;

      ScorePair scorePair(probs);

      col.push_back(std::make_pair(w,scorePair));
    }
    if(col.size()) {
      data.push_back(col);
      ShrinkToFit(data.back());
    } else break;
  }
  return !data.empty();
}

bool
ConfusionNet::
ReadFormat1(std::istream& in)
{
  Clear();
  const std::vector<FactorType>& factorOrder = m_options->input.factor_order;
  std::string line;
  if(!getline(in,line)) return 0;
  size_t s;
  if(getline(in,line)) s=atoi(line.c_str());
  else return 0;
  data.resize(s);
  for(size_t i=0; i<data.size(); ++i) {
    if(!getline(in,line)) return 0;
    std::istringstream is(line);
    if(!(is>>s)) return 0;
    std::string word;
    double prob;
    data[i].resize(s);
    for(size_t j=0; j<s; ++j)
      if(is>>word>>prob) {
        //TODO: we are only reading one prob from this input format, should read many... but this function is unused anyway. -JS
        data[i][j].second.denseScores = std::vector<float> (1);
        data[i][j].second.denseScores.push_back((float) log(prob));
        if(data[i][j].second.denseScores[0]<0) {
          VERBOSE(1, "WARN: neg costs: "<<data[i][j].second.denseScores[0]<<" -> set to 0\n");
          data[i][j].second.denseScores[0]=0.0;
        }
        // String2Word(word,data[i][j].first,factorOrder);
        Word& w = data[i][j].first;
        w.CreateFromString(Input,factorOrder,StringPiece(word),false,false);
      } else return 0;
  }
  return !data.empty();
}

void ConfusionNet::Print(std::ostream& out) const
{
  out<<"conf net: "<<data.size()<<"\n";
  for(size_t i=0; i<data.size(); ++i) {
    out<<i<<" -- ";
    for(size_t j=0; j<data[i].size(); ++j) {
      out<<"("<<data[i][j].first.ToString()<<", ";

      // dense
      std::vector<float>::const_iterator iterDense;
      for(iterDense = data[i][j].second.denseScores.begin();
          iterDense < data[i][j].second.denseScores.end();
          ++iterDense) {
        out<<", "<<*iterDense;
      }

      // sparse
      std::map<StringPiece, float>::const_iterator iterSparse;
      for(iterSparse = data[i][j].second.sparseScores.begin();
          iterSparse != data[i][j].second.sparseScores.end();
          ++iterSparse) {
        out << ", " << iterSparse->first << "=" << iterSparse->second;
      }

      out<<") ";
    }
    out<<"\n";
  }
  out<<"\n\n";
}

#ifdef _WIN32
#pragma warning(disable:4716)
#endif
Phrase
ConfusionNet::
GetSubString(const Range&) const
{
  UTIL_THROW2("ERROR: call to ConfusionNet::GetSubString\n");
  //return Phrase(Input);
}

std::string
ConfusionNet::
GetStringRep(const std::vector<FactorType> /* factorsToPrint */) const  //not well defined yet
{
  TRACE_ERR("ERROR: call to ConfusionNet::GeStringRep\n");
  return "";
}
#ifdef _WIN32
#pragma warning(disable:4716)
#endif
const Word& ConfusionNet::GetWord(size_t) const
{
  UTIL_THROW2("ERROR: call to ConfusionNet::GetFactorArray\n");
}
#ifdef _WIN32
#pragma warning(default:4716)
#endif
std::ostream& operator<<(std::ostream& out,const ConfusionNet& cn)
{
  cn.Print(out);
  return out;
}

TranslationOptionCollection*
ConfusionNet::
CreateTranslationOptionCollection(ttasksptr const& ttask) const
{
  // size_t maxNoTransOptPerCoverage
  //   = ttask->options()->search.max_trans_opt_per_cov;
  // float translationOptionThreshold
  //   = ttask->options()->search.trans_opt_threshold;
  TranslationOptionCollection *rv
  = new TranslationOptionCollectionConfusionNet(ttask, *this);
  //, maxNoTransOptPerCoverage, translationOptionThreshold);
  assert(rv);
  return rv;
}

}


