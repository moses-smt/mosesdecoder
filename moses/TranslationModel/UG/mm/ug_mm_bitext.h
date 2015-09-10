// -*- c++ -*-
// don't include this file directly! it is included by ug_bitext.h

namespace sapt
{
  template<typename TKN>
  class mmBitext : public Bitext<TKN>
  {
    void load_document_map(std::string const& fname);
  public:
    void open(std::string const base, std::string const L1, std::string L2);
    mmBitext();
  };

  template<typename TKN>
  mmBitext<TKN>::
  mmBitext()
    : Bitext<TKN>(new mmTtrack<TKN>(), new mmTtrack<TKN>(), new mmTtrack<char>(),
		  new TokenIndex(), new TokenIndex(),
		  new mmTSA<TKN>(), new mmTSA<TKN>())
  {};

  template<typename TKN>
  void
  mmBitext<TKN>::
  load_document_map(std::string const& fname)
  {
    std::ifstream docmap(fname.c_str());
    // the docmap file should list the documents in the corpus
    // in the order in which they appear with one line per document:
    // <docname> <number of lines / sentences>
    //
    // in the future, we might also allow listing documents with
    // sentence ranges.
    std::string buffer,docname; size_t a=0,b;
    this->m_sid2docid.reset(new std::vector<id_type>(this->T1->size()));
    while(getline(docmap,buffer))
      {
	std::istringstream line(buffer);
	if (!(line>>docname)) continue; // empty line
	if (docname.size() && docname[0] == '#') continue; // comment
	size_t docid = this->m_docname2docid.size();
	this->m_docname2docid[docname] = docid;
	this->m_docname.push_back(docname);
	line >> b;
#ifndef NO_MOSES
	VERBOSE(3, "DOCUMENT MAP " << docname << " " << a << "-" << b+a << std::endl);
#endif
	for (b += a; a < b; ++a)
	  (*this->m_sid2docid)[a] = docid;
      }
    UTIL_THROW_IF2(b != this->T1->size(),
		   "Document map doesn't match corpus!");
  }

  template<typename TKN>
  void
  mmBitext<TKN>::
  open(std::string const base, std::string const L1, std::string L2)
  {
    mmTtrack<TKN>&  t1 = *reinterpret_cast<mmTtrack<TKN>*>(this->T1.get());
    mmTtrack<TKN>&  t2 = *reinterpret_cast<mmTtrack<TKN>*>(this->T2.get());
    mmTtrack<char>& tx = *reinterpret_cast<mmTtrack<char>*>(this->Tx.get());
    t1.open(base+L1+".mct");
    t2.open(base+L2+".mct");
    tx.open(base+L1+"-"+L2+".mam");
    this->V1->open(base+L1+".tdx"); this->V1->iniReverseIndex();
    this->V2->open(base+L2+".tdx"); this->V2->iniReverseIndex();
    mmTSA<TKN>& i1 = *reinterpret_cast<mmTSA<TKN>*>(this->I1.get());
    mmTSA<TKN>& i2 = *reinterpret_cast<mmTSA<TKN>*>(this->I2.get());
    i1.open(base+L1+".sfa", this->T1);
    i2.open(base+L2+".sfa", this->T2);
    assert(this->T1->size() == this->T2->size());

    std::string docmapfile = base+"dmp";
    if (!access(docmapfile.c_str(),F_OK))
      load_document_map(docmapfile);
  }

}

