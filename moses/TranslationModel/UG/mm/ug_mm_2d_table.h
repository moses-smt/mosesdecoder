// -*- c++ -*-
// (c) 2007-2012 Ulrich Germann
#ifndef __ug_mm_2d_table_h
#define __ug_mm_2d_table_h
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>
#include "tpt_typedefs.h"
#include "tpt_pickler.h"
#include "ug_typedefs.h"
namespace bio=boost::iostreams;
namespace ugdiss
{
  using namespace std;
  template<typename OFFSET, typename ID, typename VAL, typename INIT>
  class
  mm2dTable
  {
  public:
    struct Cell
    {
      ID   id;
      VAL val;

      bool 
      operator<(ID const otherId) const
      {
        return id < otherId;
      }
        
      bool
      operator<(Cell const& other) const
      {
        return id < other.id;
      }

      struct SortDescendingByValue
      {
        bool operator()(Cell const& a, Cell const& b) const
        {
          return a.val > b.val;
        }
      };
    };

    struct Row
    {
      Cell const* start;
      Cell const* stop;
      VAL operator[](ID key) const;
    };

    Cell* data;
    VAL   *M1, *M2;
    OFFSET * index;
    ID numRows;
    ID numCols;
    boost::shared_ptr<bio::mapped_file> file;

    VAL m1(ID key) const 
    { 
      return (key < numRows) ? M1[key] : INIT(0); 
    }

    VAL m2(ID key) const
    {
      return (key < numCols) ? M2[key] : INIT(0); 
    }


    void open(string fname);
    void close();

    Row operator[](ID key) const;

    mm2dTable(string const fname="") { if (!fname.empty()) open(fname); };
    ~mm2dTable() { file.reset(); };
  };

  template<typename OFFSET, typename ID, typename VAL, typename INIT>
  typename mm2dTable<OFFSET,ID,VAL,INIT>::Row
  mm2dTable<OFFSET,ID,VAL,INIT>::
  operator[](ID key) const
  {
    Row ret;
    if (key < numRows)
      {
        ret.start = data+index[key];
        ret.stop  = data+index[key+1];
      }
    else
      ret.start = ret.stop = data+index[key+1];
    return ret;
  }

  template<typename OFFSET, typename ID, typename VAL, typename INIT>
  VAL
  mm2dTable<OFFSET,ID,VAL,INIT>::
  Row::
  operator[](ID key) const
  {
    if (start==stop) return INIT(0);
    Cell const* c = lower_bound(start,stop,key);
    return (c != stop && c->id == key ? c->val : INIT(0));
  }
  
  template<typename OFFSET, typename ID, typename VAL, typename INIT>
  void
  mm2dTable<OFFSET,ID,VAL,INIT>::
  open(string fname)
  {
    // cout << "opening " << fname << " at " << __FILE__ << ":" << __LINE__ << endl;
    if (access(fname.c_str(),R_OK))
      {
        cerr << "[" << __FILE__ << ":" << __LINE__ <<"] FATAL ERROR: "
             << "file '" << fname << " is not accessible." << endl;
        exit(1);
      }
    file.reset(new bio::mapped_file());
    file->open(fname,ios::in|ios::out);
    if (!file->is_open())
      {
	cerr << "Error opening file " << fname << endl;
	assert(0);
      }
    char* p = file->data();
    filepos_type offset = *reinterpret_cast<filepos_type*>(p);
    index = reinterpret_cast<OFFSET*>(p+offset); p += sizeof(offset);
    numRows = *reinterpret_cast<ID const*>(p);   p += sizeof(id_type);
    numCols = *reinterpret_cast<ID const*>(p);   p += sizeof(id_type);
    data = reinterpret_cast<Cell*>(p);
    // cout << numRows << " rows; " << numCols << " columns " << endl;
    M1 = reinterpret_cast<VAL*>(index+numRows+1);
    M2 = M1+numRows;
    //    cout << "Table " << fname << " has " << numRows << " rows and " 
    //         << numCols << " columns." << endl;
    //     cout << "File size is " << file.size()*1024 << " bytes; "; 
    //     cout << "M2 starts " << (reinterpret_cast<char const*>(M2) - file.data()) 
    //          << " bytes into the file" << endl;
    // cout << M2[0] << endl;
  }

  template<
    typename OFFSET, // integer type of file offsets
    typename ID,     // integer type of column ids
    typename VAL,    // type of cell values
    typename INIT,   // INIT(0) initializes default values
    typename ICONT   // inner container type
    >
  void
  write_mm_2d_table(ostream& out, vector<ICONT> const& T, 
                    vector<VAL> const* m1    = NULL, 
                    vector<VAL> const* m2    = NULL)
  {
    assert(T.size());
    typedef typename ICONT::const_iterator iter;

    // compute marginals if necessary
    vector<VAL> m1x,m2x;
    if (!m1)
      {
        m1x.resize(T.size(),INIT(0));
        for (size_t r = 0; r < T.size(); ++r)
          for (iter c = T.at(r).begin(); c != T.at(r).end(); ++c)
            m1x[r] = m1x[r] + c->second;
        m1 = &m1x;
      }
    if (!m2)
      {
        for (size_t r = 0; r < T.size(); ++r)
          for (iter c = T.at(r).begin(); c != T.at(r).end(); ++c)
            {
              while (c->first >= m2x.size())
                m2x.push_back(INIT(0));
              m2x[c->first] = m2x[c->first] + c->second;
            }
        m2 = &m2x;
      }

    filepos_type idxOffset=0;
    numwrite(out,idxOffset); // place holder, we'll return here at the end
    numwrite(out,id_type(m1->size())); // number of rows
    numwrite(out,id_type(m2->size())); // number of columns

    // write actual table
    vector<OFFSET> index;
    size_t ctr =0;
    index.reserve(m1->size()+1);
    for (ID r = 0; r < ID(T.size()); ++r)
      {
        //index.push_back(out.tellp());
        index.push_back(ctr);
        ID lastId = 0;
        if (T.at(r).size())
          lastId = T.at(r).begin()->first;
        for (typename ICONT::const_iterator c = T.at(r).begin();
             c != T.at(r).end(); ++c)
          {
            ctr++;
            assert(c->first >= lastId);
            lastId = c->first;
            typename mm2dTable<OFFSET,ID,VAL,INIT>::Cell item;
            item.id  = c->first;
            item.val = c->second;
            out.write(reinterpret_cast<char const*>(&item),sizeof(item));
          }
      }
    // index.push_back(out.tellp());
    index.push_back(ctr);
    idxOffset=out.tellp();

    // write index
    for (size_t i = 0; i < index.size(); ++i)
      {
        OFFSET o = index[i]; // (index[i]-index[0])/sizeof(VAL);
        out.write(reinterpret_cast<char*>(&o),sizeof(OFFSET));
      }
   
    // write marginals
    out.write(reinterpret_cast<char const*>(&(*m1)[0]),m1->size()*sizeof(VAL));
    out.write(reinterpret_cast<char const*>(&(*m2)[0]),m2->size()*sizeof(VAL));

    out.seekp(0);
    numwrite(out,idxOffset);
  }
}
#endif
