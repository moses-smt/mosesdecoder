// -*- c++ -*-
#pragma once
#include "ug_bitext.h"

namespace Moses
{
  namespace bitext
  {
    template<typename TKN>
    class imBitext : public Bitext<TKN>
    {
      sptr<imTtrack<char> > myTx;
      sptr<imTtrack<TKN> >  myT1;
      sptr<imTtrack<TKN> >  myT2;
      sptr<imTSA<TKN> >     myI1;
      sptr<imTSA<TKN> >     myI2;
      static ThreadSafeCounter my_revision;
    public:
      size_t revision() const { return my_revision; }
      void open(string const base, string const L1, string L2);
      imBitext(sptr<TokenIndex> const& V1,
	       sptr<TokenIndex> const& V2,
	       size_t max_sample = 5000, size_t num_workers=4);
      imBitext(size_t max_sample = 5000, size_t num_workers=4);
      imBitext(imBitext const& other);

      // sptr<imBitext<TKN> >
      // add(vector<TKN> const& s1, vector<TKN> const& s2, vector<ushort> & a);

      sptr<imBitext<TKN> >
      add(vector<string> const& s1,
	  vector<string> const& s2,
	  vector<string> const& a) const;

    };

    template<typename TKN>
    ThreadSafeCounter
    imBitext<TKN>::my_revision;

    template<typename TKN>
    imBitext<TKN>::
    imBitext(size_t max_sample, size_t num_workers)
      : Bitext<TKN>(max_sample, num_workers)
    {
      this->m_default_sample_size = max_sample;
      this->V1.reset(new TokenIndex());
      this->V2.reset(new TokenIndex());
      this->V1->setDynamic(true);
      this->V2->setDynamic(true);
      ++my_revision;
    }

    template<typename TKN>
    imBitext<TKN>::
    imBitext(sptr<TokenIndex> const& v1,
	     sptr<TokenIndex> const& v2,
	     size_t max_sample, size_t num_workers)
      : Bitext<TKN>(max_sample, num_workers)
    {
      // this->default_sample_size = max_sample;
      this->V1 = v1;
      this->V2 = v2;
      this->V1->setDynamic(true);
      this->V2->setDynamic(true);
      ++my_revision;
    }


    template<typename TKN>
    imBitext<TKN>::
    imBitext(imBitext<TKN> const& other)
    {
      this->myTx = other.myTx;
      this->myT1 = other.myT1;
      this->myT2 = other.myT2;
      this->myI1 = other.myI1;
      this->myI2 = other.myI2;
      this->Tx = this->myTx;
      this->T1 = this->myT1;
      this->T2 = this->myT2;
      this->I1 = this->myI1;
      this->I2 = this->myI2;
      this->V1 = other.V1;
      this->V2 = other.V2;
      this->m_default_sample_size = other.m_default_sample_size;
      this->m_num_workers = other.m_num_workers;
      ++my_revision;
    }

    template<>
    sptr<imBitext<L2R_Token<SimpleWordId> > >
    imBitext<L2R_Token<SimpleWordId> >::
    add(vector<string> const& s1,
	vector<string> const& s2,
	vector<string> const& aln) const;

    template<typename TKN>
    sptr<imBitext<TKN> >
    imBitext<TKN>::
    add(vector<string> const& s1,
	vector<string> const& s2,
	vector<string> const& aln) const
    {
      throw "Not yet implemented";
    }

    // What's up with this function???? UG
    template<typename TKN>
    void
    imBitext<TKN>::
    open(string const base, string const L1, string L2)
    {
      mmTtrack<TKN>& t1 = *reinterpret_cast<mmTtrack<TKN>*>(this->T1.get());
      mmTtrack<TKN>& t2 = *reinterpret_cast<mmTtrack<TKN>*>(this->T2.get());
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
    }

  }
}
