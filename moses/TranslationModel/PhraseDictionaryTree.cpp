// $Id$
// vim:tabstop=2
#include "moses/FeatureVector.h"
#include "moses/TranslationModel/PhraseDictionaryTree.h"
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>


namespace Moses
{

template<typename T>
std::ostream& operator<<(std::ostream& out,const std::vector<T>& x)
{
  out<<x.size()<<" ";
  typename std::vector<T>::const_iterator iend=x.end();
  for(typename std::vector<T>::const_iterator i=x.begin(); i!=iend; ++i)
    out<<*i<<' ';
  return out;
}


class TgtCand
{
  IPhrase e;
  Scores sc;
  std::string m_alignment;
  IPhrase fnames;
  std::vector<FValue> fvalues;

  static const float SPARSE_FLAG;

public:
  TgtCand() {}

  TgtCand(const IPhrase& a, const Scores& b , const std::string& alignment)
    : e(a)
    , sc(b)
    , m_alignment(alignment) {
  }

  TgtCand(const IPhrase& a,const Scores& b) : e(a),sc(b) {}

  TgtCand(FILE* f) {
    readBin(f);
  }


  void writeBin(FILE* f) const {
    fWriteVector(f,e);
    //This is a bit ugly, but if there is a sparse vector, add
    //an extra score with value 100. Can the last score be 100?
    //Probably not, since scores are probabilities and phrase penalty.
    if (fnames.size()) {
      Scores sc_copy(sc);
      sc_copy.push_back(SPARSE_FLAG);
      fWriteVector(f,sc_copy);
      fWriteVector(f,fnames);
      fWriteVector(f,fvalues);
    } else {
      fWriteVector(f,sc);
    }
  }

  void readBin(FILE* f) {
    fReadVector(f,e);
    fReadVector(f,sc);
    if (sc.back() == 100) {
      sc.pop_back();
      fReadVector(f,fnames);
      fReadVector(f,fvalues);
    }
  }

  void writeBinWithAlignment(FILE* f) const {
    writeBin(f);
    fWriteString(f, m_alignment.c_str(), m_alignment.size());
  }

  void readBinWithAlignment(FILE* f) {
    readBin(f);
    fReadString(f, m_alignment);
  }

  const IPhrase& GetPhrase() const {
    return e;
  }
  const Scores& GetScores() const {
    return sc;
  }
  const std::string& GetAlignment() const {
    return m_alignment;
  }

  const IPhrase& GetFeatureNames() const {
    return fnames;
  }

  const std::vector<FValue> GetFeatureValues() const {
    return fvalues;
  }

  void SetFeatures(const IPhrase& names, const std::vector<FValue>& values) {
    UTIL_THROW_IF2(names.size() != values.size(), "Error");
    fnames = names;
    fvalues = values;
  }
};

const float TgtCand::SPARSE_FLAG = 100;


class TgtCands : public std::vector<TgtCand>
{
  typedef std::vector<TgtCand> MyBase;
public:
  TgtCands() : MyBase() {}

  void writeBin(FILE* f) const {
    unsigned s=size();
    fWrite(f,s);
    for(size_t i=0; i<s; ++i) MyBase::operator[](i).writeBin(f);
  }

  void writeBinWithAlignment(FILE* f) const {
    unsigned s=size();
    fWrite(f,s);
    for(size_t i=0; i<s; ++i) MyBase::operator[](i).writeBinWithAlignment(f);
  }

  void readBin(FILE* f) {
    unsigned s;
    fRead(f,s);
    resize(s);
    for(size_t i=0; i<s; ++i) MyBase::operator[](i).readBin(f);
  }

  void readBinWithAlignment(FILE* f) {
    unsigned s;
    fRead(f,s);
    resize(s);
    for(size_t i=0; i<s; ++i) MyBase::operator[](i).readBinWithAlignment(f);
  }
};


PhraseDictionaryTree::PrefixPtr::operator bool() const
{
  return imp && imp->isValid();
}

typedef LVoc<std::string> WordVoc;


class PDTimp
{
public:
  typedef PrefixTreeF<LabelId,OFF_T> PTF;
  typedef FilePtr<PTF> CPT;
  typedef std::vector<CPT> Data;


  Data data;
  std::vector<OFF_T> srcOffsets;

  FILE *os,*ot;
  WordVoc sv;
  WordVoc tv;

  ObjectPool<PPimp> pPool;
  // a comparison with the Boost MemPools might be useful

  bool needwordalign, haswordAlign;
  bool printwordalign;

  PDTimp() : os(0),ot(0), printwordalign(false) {
    PTF::setDefault(InvalidOffT);
  }
  ~PDTimp() {
    if(os) fClose(os);
    if(ot) fClose(ot);
    FreeMemory();
  }

  inline void NeedAlignmentInfo(bool a) {
    needwordalign=a;
  }
  inline bool NeedAlignmentInfo() {
    return needwordalign;
  };
  inline void HasAlignmentInfo(bool a) {
    haswordAlign=a;
  }
  inline bool HasAlignmentInfo() {
    return haswordAlign;
  };

  inline void PrintWordAlignment(bool a) {
    printwordalign=a;
  };
  inline bool PrintWordAlignment() {
    return printwordalign;
  };

  void FreeMemory() {
    for(Data::iterator i=data.begin(); i!=data.end(); ++i) (*i).free();
    pPool.reset();
  }

  int Read(const std::string& fn);

  void GetTargetCandidates(const IPhrase& f,TgtCands& tgtCands) {
    if(f.empty()) return;
    if(f[0]>=data.size()) return;
    if(!data[f[0]]) return;
    assert(data[f[0]]->findKey(f[0])<data[f[0]]->size());
    OFF_T tCandOffset=data[f[0]]->find(f);
    if(tCandOffset==InvalidOffT) return;
    fSeek(ot,tCandOffset);

    if (HasAlignmentInfo())
      tgtCands.readBinWithAlignment(ot);
    else
      tgtCands.readBin(ot);
  }

  typedef PhraseDictionaryTree::PrefixPtr PPtr;

  void GetTargetCandidates(PPtr p,TgtCands& tgtCands) {
    UTIL_THROW_IF2(p == NULL, "Error");

    if(p.imp->isRoot()) return;
    OFF_T tCandOffset=p.imp->ptr()->getData(p.imp->idx);
    if(tCandOffset==InvalidOffT) return;
    fSeek(ot,tCandOffset);
    if (HasAlignmentInfo())
      tgtCands.readBinWithAlignment(ot);
    else
      tgtCands.readBin(ot);
  }

  void PrintTgtCand(const TgtCands& tcands,std::ostream& out) const;

  // convert target candidates from internal data structure to the external one
  void ConvertTgtCand(const TgtCands& tcands,std::vector<StringTgtCand>& extTgtCands,
                      std::vector<std::string>* wa) const {
    extTgtCands.reserve(tcands.size());
    for(TgtCands::const_iterator iter=tcands.begin(); iter!=tcands.end(); ++iter) {
      const TgtCand &intTgtCand = *iter;

      extTgtCands.push_back(StringTgtCand());
      StringTgtCand &extTgtCand = extTgtCands.back();

      const IPhrase& iphrase = intTgtCand.GetPhrase();

      extTgtCand.tokens.reserve(iphrase.size());
      for(size_t j=0; j<iphrase.size(); ++j) {
        extTgtCand.tokens.push_back(&tv.symbol(iphrase[j]));
      }
      extTgtCand.scores = intTgtCand.GetScores();
      const IPhrase& fnames = intTgtCand.GetFeatureNames();
      for (size_t j = 0; j < fnames.size(); ++j) {
        extTgtCand.fnames.push_back(&tv.symbol(fnames[j]));
      }
      extTgtCand.fvalues = intTgtCand.GetFeatureValues();
      if (wa) wa->push_back(intTgtCand.GetAlignment());
    }
  }

  PPtr GetRoot() {
    return PPtr(pPool.get(PPimp(0,0,1)));
  }

  PPtr Extend(PPtr p,const std::string& w) {
	UTIL_THROW_IF2(p == NULL, "Error");

    if(w.empty() || w==EPSILON) return p;

    LabelId wi=sv.index(w);

    if(wi==InvalidLabelId) return PPtr(); // unknown word
    else if(p.imp->isRoot()) {
      if(wi<data.size() && data[wi]) {
        const void* ptr = data[wi]->findKeyPtr(wi);
        UTIL_THROW_IF2(ptr == NULL, "Error");

        return PPtr(pPool.get(PPimp(data[wi],data[wi]->findKey(wi),0)));
      }
    } else if(PTF const* nextP=p.imp->ptr()->getPtr(p.imp->idx)) {
      return PPtr(pPool.get(PPimp(nextP,nextP->findKey(wi),0)));
    }

    return PPtr();
  }

  WordVoc* ReadVoc(const std::string& filename);
};


////////////////////////////////////////////////////////////
//
// member functions of PDTimp
//
////////////////////////////////////////////////////////////

int PDTimp::Read(const std::string& fn)
{
  std::string ifs, ift, ifi, ifsv, iftv;

  HasAlignmentInfo(FileExists(fn+".binphr.srctree.wa"));

  if (NeedAlignmentInfo() && !HasAlignmentInfo()) {
    //    ERROR
    std::stringstream strme;
    strme << "You are asking for word alignment but the binary phrase table does not contain any alignment info. Please check if you had generated the correct phrase table with word alignment (.wa)\n";
    UserMessage::Add(strme.str());
    return false;
  }

  if (HasAlignmentInfo()) {
    ifs=fn+".binphr.srctree.wa";
    ift=fn+".binphr.tgtdata.wa";
  } else {
    ifs=fn+".binphr.srctree";
    ift=fn+".binphr.tgtdata";
  }

  ifi=fn+".binphr.idx";
  ifsv=fn+".binphr.srcvoc";
  iftv=fn+".binphr.tgtvoc";

  FILE *ii=fOpen(ifi.c_str(),"rb");
  fReadVector(ii,srcOffsets);
  fClose(ii);

  os=fOpen(ifs.c_str(),"rb");
  ot=fOpen(ift.c_str(),"rb");

  data.resize(srcOffsets.size());
  for(size_t i=0; i<data.size(); ++i)
    data[i]=CPT(os,srcOffsets[i]);

  sv.Read(ifsv);
  tv.Read(iftv);

  TRACE_ERR("binary phrasefile loaded, default OFF_T: "<<PTF::getDefault()
            <<"\n");
  return 1;
}

void PDTimp::PrintTgtCand(const TgtCands& tcand,std::ostream& out) const
{
  for(size_t i=0; i<tcand.size(); ++i) {

    Scores sc=tcand[i].GetScores();
    std::string	trgAlign = tcand[i].GetAlignment();

    const IPhrase& iphr=tcand[i].GetPhrase();

    out << i << " -- " << sc << " -- ";
    for(size_t j=0; j<iphr.size(); ++j)			out << tv.symbol(iphr[j])<<" ";
    out<< " -- " << trgAlign;
    out << std::endl;
  }
}

////////////////////////////////////////////////////////////
//
// member functions of PhraseDictionaryTree
//
////////////////////////////////////////////////////////////

PhraseDictionaryTree::PhraseDictionaryTree()
  : imp(new PDTimp)
{
  if(sizeof(OFF_T)!=8) {
    TRACE_ERR("ERROR: size of type 'OFF_T' has to be 64 bit!\n"
              "In gcc, use compiler settings '-D_FILE_OFFSET_BITS=64 -D_LARGE_FILES'\n"
              " -> abort \n\n");
    abort();
  }
}

PhraseDictionaryTree::~PhraseDictionaryTree()
{
  delete imp;
}

void PhraseDictionaryTree::NeedAlignmentInfo(bool a)
{
  imp->NeedAlignmentInfo(a);
};
void PhraseDictionaryTree::PrintWordAlignment(bool a)
{
  imp->PrintWordAlignment(a);
};
bool PhraseDictionaryTree::PrintWordAlignment()
{
  return imp->PrintWordAlignment();
};

void PhraseDictionaryTree::FreeMemory() const
{
  imp->FreeMemory();
}


void PhraseDictionaryTree::
GetTargetCandidates(const std::vector<std::string>& src,
                    std::vector<StringTgtCand>& rv) const
{
  IPhrase f(src.size());
  for(size_t i=0; i<src.size(); ++i) {
    f[i]=imp->sv.index(src[i]);
    if(f[i]==InvalidLabelId) return;
  }

  TgtCands tgtCands;
  imp->GetTargetCandidates(f,tgtCands);
  imp->ConvertTgtCand(tgtCands,rv,NULL);
}

void PhraseDictionaryTree::
GetTargetCandidates(const std::vector<std::string>& src,
                    std::vector<StringTgtCand>& rv,
                    std::vector<std::string>& wa) const
{
  IPhrase f(src.size());
  for(size_t i=0; i<src.size(); ++i) {
    f[i]=imp->sv.index(src[i]);
    if(f[i]==InvalidLabelId) return;
  }

  TgtCands tgtCands;
  imp->GetTargetCandidates(f,tgtCands);
  imp->ConvertTgtCand(tgtCands,rv,&wa);
}


void PhraseDictionaryTree::
PrintTargetCandidates(const std::vector<std::string>& src,
                      std::ostream& out) const
{
  IPhrase f(src.size());
  for(size_t i=0; i<src.size(); ++i) {
    f[i]=imp->sv.index(src[i]);
    if(f[i]==InvalidLabelId) {
      TRACE_ERR("the source phrase '"<<src<<"' contains an unknown word '"
                <<src[i]<<"'\n");
      return;
    }
  }

  TgtCands tcand;
  imp->GetTargetCandidates(f,tcand);
  imp->PrintTgtCand(tcand,out);
}

int PhraseDictionaryTree::Create(std::istream& inFile,const std::string& out)
{
  std::string line;
  size_t count = 0;

  std::string ofn(out+".binphr.srctree"),
      oft(out+".binphr.tgtdata"),
      ofi(out+".binphr.idx"),
      ofsv(out+".binphr.srcvoc"),
      oftv(out+".binphr.tgtvoc");

  if (PrintWordAlignment()) {
    ofn+=".wa";
    oft+=".wa";
  }

  FILE *os=fOpen(ofn.c_str(),"wb"),
        *ot=fOpen(oft.c_str(),"wb");

  typedef PrefixTreeSA<LabelId,OFF_T> PSA;
  PSA *psa=new PSA;
  PSA::setDefault(InvalidOffT);

  LabelId currFirstWord=InvalidLabelId;
  IPhrase currF;
  TgtCands tgtCands;
  std::vector<OFF_T> vo;
  size_t lnc=0;
  size_t numElement = NOT_FOUND; // 3=old format, 5=async format which include word alignment info
  size_t missingAlignmentCount = 0;

  while(getline(inFile, line)) {
    ++lnc;

    std::vector<std::string> tokens = TokenizeMultiCharSeparator( line , "|||" );

    if (numElement == NOT_FOUND) {
      // init numElement
      numElement = tokens.size();
      UTIL_THROW_IF2(numElement < (PrintWordAlignment()?4:3),
    		  "Format error");
    }

    if (tokens.size() != numElement) {
      std::stringstream strme;
      strme << "Syntax error at line " << lnc  << " : " << line;
      UserMessage::Add(strme.str());
      abort();
    }

    const std::string &sourcePhraseString	=tokens[0]
                                           ,&targetPhraseString=tokens[1]
                                               ,&scoreString				= tokens[2];
    const std::string empty;
    const std::string &alignmentString = PrintWordAlignment() ? tokens[3] : empty;
    const std::string sparseFeatureString = tokens.size() > 5 ? tokens[5] : empty;
    IPhrase f,e;
    Scores sc;

    if (PrintWordAlignment() && alignmentString == " ") ++missingAlignmentCount;

    std::vector<std::string> wordVec = Tokenize(sourcePhraseString);
    for (size_t i = 0 ; i < wordVec.size() ; ++i)
      f.push_back(imp->sv.add(wordVec[i]));

    wordVec = Tokenize(targetPhraseString);
    for (size_t i = 0 ; i < wordVec.size() ; ++i)
      e.push_back(imp->tv.add(wordVec[i]));

    //			while(is>>w && w!="|||") sc.push_back(atof(w.c_str()));
    // Mauro: to handle 0 probs in phrase tables
    std::vector<float> scoreVector = Tokenize<float>(scoreString);
    for (size_t i = 0 ; i < scoreVector.size() ; ++i) {
      float tmp = scoreVector[i];
      sc.push_back(((tmp>0.0)?tmp:(float)1.0e-38));
    }

    if(f.empty()) {
      TRACE_ERR("WARNING: empty source phrase in line '"<<line<<"'\n");
      continue;
    }

    if(currFirstWord==InvalidLabelId) currFirstWord=f[0];
    if(currF.empty()) {
      ++count;
      currF=f;
      // insert src phrase in prefix tree
      UTIL_THROW_IF2(psa == NULL, "Error");

      PSA::Data& d=psa->insert(f);
      if(d==InvalidOffT) d=fTell(ot);
      else {
        TRACE_ERR("ERROR: source phrase already inserted (A)!\nline(" << lnc << "): '"
                  <<line<<"'\nf: "<<f<<"\n");
        abort();
      }
    }

    IPhrase fnames;
    std::vector<FValue> fvalues;
    if (!sparseFeatureString.empty()) {
      std::vector<std::string> sparseTokens = Tokenize(sparseFeatureString);
      if (sparseTokens.size() % 2 != 0) {
        TRACE_ERR("ERROR: incorrectly formatted sparse feature string: " <<
                  sparseFeatureString << std::endl);
        abort();
      }
      for (size_t i = 0; i < sparseTokens.size(); i+=2) {
        fnames.push_back(imp->tv.add(sparseTokens[i]));
        fvalues.push_back(Scan<FValue>(sparseTokens[i+1]));
      }
    }

    if(currF!=f) {
      // new src phrase
      currF=f;
      if (PrintWordAlignment())
        tgtCands.writeBinWithAlignment(ot);
      else
        tgtCands.writeBin(ot);
      tgtCands.clear();

      if(++count%10000==0) {
        TRACE_ERR(".");
        if(count%500000==0) TRACE_ERR("[phrase:"<<count<<"]\n");
      }

      if(f[0]!=currFirstWord) {
        // write src prefix tree to file and clear
        PTF pf;
        if(currFirstWord>=vo.size())
          vo.resize(currFirstWord+1,InvalidOffT);
        vo[currFirstWord]=fTell(os);
        pf.create(*psa,os);
        // clear
        delete psa;
        psa=new PSA;
        currFirstWord=f[0];
      }

      // insert src phrase in prefix tree
      UTIL_THROW_IF2(psa == NULL, "Error");

      PSA::Data& d=psa->insert(f);
      if(d==InvalidOffT) d=fTell(ot);
      else {
        TRACE_ERR("ERROR: xsource phrase already inserted (B)!\nline(" << lnc << "): '"
                  <<line<<"'\nf: "<<f<<"\n");
        abort();
      }
    }
    tgtCands.push_back(TgtCand(e,sc, alignmentString));
    UTIL_THROW_IF2(currFirstWord == InvalidLabelId,
    		"Uninitialize word");
    tgtCands.back().SetFeatures(fnames, fvalues);
  }
  if (PrintWordAlignment())
    tgtCands.writeBinWithAlignment(ot);
  else
    tgtCands.writeBin(ot);
  tgtCands.clear();

  PTF pf;
  if(currFirstWord>=vo.size()) vo.resize(currFirstWord+1,InvalidOffT);
  vo[currFirstWord]=fTell(os);
  pf.create(*psa,os);
  delete psa;
  psa=0;

  TRACE_ERR("distinct source phrases: "<<count
            <<" distinct first words of source phrases: "<<vo.size()
            <<" number of phrase pairs (line count): "<<lnc
            <<"\n");

  if ( PrintWordAlignment()) {
    TRACE_ERR("Count of lines with missing alignments: " <<
              missingAlignmentCount << "/" << lnc << "\n");
  }

  fClose(os);
  fClose(ot);

  std::vector<size_t> inv;
  for(size_t i=0; i<vo.size(); ++i)
    if(vo[i]==InvalidOffT) inv.push_back(i);

  if(inv.size()) {
    TRACE_ERR("WARNING: there are src voc entries with no phrase "
              "translation: count "<<inv.size()<<"\n"
              "There exists phrase translations for "<<vo.size()-inv.size()
              <<" entries\n");
  }

  FILE *oi=fOpen(ofi.c_str(),"wb");
  fWriteVector(oi,vo);
  fClose(oi);

  imp->sv.Write(ofsv);
  imp->tv.Write(oftv);

  return 1;
}


int PhraseDictionaryTree::Read(const std::string& fn)
{
  TRACE_ERR("size of OFF_T "<<sizeof(OFF_T)<<"\n");
  return imp->Read(fn);
}


PhraseDictionaryTree::PrefixPtr PhraseDictionaryTree::GetRoot() const
{
  return imp->GetRoot();
}

PhraseDictionaryTree::PrefixPtr
PhraseDictionaryTree::Extend(PrefixPtr p, const std::string& w) const
{
  return imp->Extend(p,w);
}

void PhraseDictionaryTree::PrintTargetCandidates(PrefixPtr p,std::ostream& out) const
{

  TgtCands tcand;
  imp->GetTargetCandidates(p,tcand);
  out<<"there are "<<tcand.size()<<" target candidates\n";
  imp->PrintTgtCand(tcand,out);
}

void PhraseDictionaryTree::
GetTargetCandidates(PrefixPtr p,
                    std::vector<StringTgtCand>& rv) const
{
  TgtCands tcands;
  imp->GetTargetCandidates(p,tcands);
  imp->ConvertTgtCand(tcands,rv,NULL);
}

void PhraseDictionaryTree::
GetTargetCandidates(PrefixPtr p,
                    std::vector<StringTgtCand>& rv,
                    std::vector<std::string>& wa) const
{
  TgtCands tcands;
  imp->GetTargetCandidates(p,tcands);
  imp->ConvertTgtCand(tcands,rv,&wa);
}

}

