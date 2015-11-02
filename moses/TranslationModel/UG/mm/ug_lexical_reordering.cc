#include "ug_lexical_reordering.h"
#include "moses/Util.h"
namespace sapt
{
  using namespace std;

  LRModel::ReorderingType po_other = LRModel::NONE;
  // check if min and max in the aligmnet vector v are within the
  // bounds LFT and RGT and update the actual bounds L and R; update
  // the total count of alignment links in the underlying phrase
  // pair
  bool
  check(vector<ushort> const& v, // alignment row/column
	size_t const LFT, size_t const RGT, // hard limits
	ushort& L, ushort& R, size_t& count) // current bounds, count
  {
    if (v.size() == 0) return 0;
    if (L > v.front() && (L=v.front()) < LFT) return false;
    if (R < v.back()  && (R=v.back())  > RGT) return false;
    count += v.size();
    return true;
  }

  /// return number of alignment points in box, -1 on failure
  int
  expand_block(vector<vector<ushort> > const& row2col,
	       vector<vector<ushort> > const& col2row,
	       size_t       row, size_t       col, // seed coordinates
	       size_t const TOP, size_t const LFT, // hard limits
	       size_t const BOT, size_t const RGT, // hard limits
	       ushort* top = NULL, ushort* lft = NULL,
	       ushort* bot = NULL, ushort* rgt = NULL) // store results
  {
    if (row < TOP || row > BOT || col < LFT || col > RGT) return -1;
    UTIL_THROW_IF2(row >= row2col.size(), "out of bounds");
    UTIL_THROW_IF2(col >= col2row.size(), "out of bounds");

    // ====================================================
    // tables grow downwards, so TOP is smaller than BOT!
    // ====================================================

    ushort T, L, B, R; // box dimensions

    // if we start on an empty cell, search for the first alignment point
    if (row2col[row].size() == 0 && col2row[col].size() == 0)
      {
	if      (row == TOP) while (row < BOT && !row2col[++row].size());
	else if (row == BOT) while (row > TOP && !row2col[--row].size());

	if      (col == LFT) while (col < RGT && !col2row[++col].size());
	else if (col == RGT) while (col > RGT && !col2row[--col].size());

	if (row2col[row].size() == 0 && col2row[col].size() == 0)
	  return 0;
      }
    if (row2col[row].size() == 0)
      row = col2row[col].front();
    if (col2row[col].size() == 0)
      col = row2col[row].front();

    if ((T = col2row[col].front()) < TOP) return -1;
    if ((B = col2row[col].back())  > BOT) return -1;
    if ((L = row2col[row].front()) < LFT) return -1;
    if ((R = row2col[row].back())  > RGT) return -1;

    if (B == T && R == L) return 1;

    // start/end of row / column coverage:
    ushort rs = row, re = row, cs = col, ce = col;
    int ret = row2col[row].size();
    for (size_t tmp = 1; tmp; ret += tmp)
      {
	tmp = 0;;
	while (rs>T) if (!check(row2col[--rs],LFT,RGT,L,R,tmp)) return -1;
	while (re<B) if (!check(row2col[++re],LFT,RGT,L,R,tmp)) return -1;
	while (cs>L) if (!check(col2row[--cs],TOP,BOT,T,B,tmp)) return -1;
	while (ce<R) if (!check(col2row[++ce],TOP,BOT,T,B,tmp)) return -1;
      }
    if (top) *top = T;
    if (bot) *bot = B;
    if (lft) *lft = L;
    if (rgt) *rgt = R;
    return ret;
  }

  // LRModel::ReorderingType
  sapt::PhraseOrientation
  find_po_fwd(vector<vector<ushort> >& a1,
	      vector<vector<ushort> >& a2,
	      size_t s1, size_t e1,
	      size_t s2, size_t e2)
  {
    if (e2 == a2.size()) // end of target sentence
      return LRModel::M;
    size_t y = e2, L = e2, R = a2.size()-1; // won't change
    size_t x = e1, T = e1, B = a1.size()-1;
    if (e1 < a1.size() && expand_block(a1,a2,x,y,T,L,B,R) >= 0)
      return LRModel::M;
    B = x = s1-1; T = 0;
    if (s1 && expand_block(a1,a2,x,y,T,L,B,R) >= 0)
      return LRModel::S;
    while (e2 < a2.size() && a2[e2].size() == 0) ++e2;
    if (e2 == a2.size()) // should never happen, actually
      return LRModel::NONE;
    if (a2[e2].back() < s1)
      return LRModel::DL;
    if (a2[e2].front() >= e1)
      return LRModel::DR;
    return LRModel::NONE;
  }


  // LRModel::ReorderingType
  PhraseOrientation
  find_po_bwd(vector<vector<ushort> >& a1,
	      vector<vector<ushort> >& a2,
	      size_t s1, size_t e1,
	      size_t s2, size_t e2)
  {
    if (s1 == 0 && s2 == 0) return LRModel::M;
    if (s2 == 0) return LRModel::DR;
    if (s1 == 0) return LRModel::DL;
    size_t y = s2-1, L = 0, R = s2-1; // won't change
    size_t x = s1-1, T = 0, B = s1-1;
    if (expand_block(a1,a2,x,y,T,L,B,R) >= 0)
      return LRModel::M;
    T = x = e1; B = a1.size()-1;
    if (expand_block(a1,a2,x,y,T,L,B,R) >= 0)
      return LRModel::S;
    while (s2-- && a2[s2].size() == 0);

    LRModel::ReorderingType ret;
    ret = (a2[s2].size()  ==  0 ? po_other :
	   a2[s2].back()   < s1 ? LRModel::DR :
	   a2[s2].front() >= e1 ? LRModel::DL :
	   po_other);
#if 0
    cout << "s1=" << s1 << endl;
    cout << "s2=" << s2x << "=>" << s2 << endl;
    cout << "e1=" << e1 << endl;
    cout << "e2=" << e2 << endl;
    cout << "a2[s2].size()=" << a2[s2].size() << endl;
    cout << "a2[s2].back()=" << a2[s2].back() << endl;
    cout << "a2[s2].front()=" << a2[s2].front() << endl;
    cout << "RETURNING " << ret << endl;
#endif
    return ret;
  }

} // namespace sapt

