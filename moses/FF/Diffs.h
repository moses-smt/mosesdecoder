#ifndef moses_Diffs_h
#define moses_Diffs_h

#include <cmath>

namespace Moses
{

typedef char Diff;
typedef std::vector<Diff> Diffs;

template <class Sequence, class Pred>
void CreateDiffRec(size_t** c,
                   const Sequence &s1,
                   const Sequence &s2,
                   size_t start,
                   size_t i,
                   size_t j,
                   Diffs& diffs,
                   Pred pred)
{
  if(i > 0 && j > 0 && pred(s1[i - 1 + start], s2[j - 1 + start])) {
    CreateDiffRec(c, s1, s2, start, i - 1, j - 1, diffs, pred);
    diffs.push_back(Diff('m'));
  } else if(j > 0 && (i == 0 || c[i][j-1] >= c[i-1][j])) {
    CreateDiffRec(c, s1, s2, start, i, j-1, diffs, pred);
    diffs.push_back(Diff('i'));
  } else if(i > 0 && (j == 0 || c[i][j-1] < c[i-1][j])) {
    CreateDiffRec(c, s1, s2, start, i-1, j, diffs, pred);
    diffs.push_back(Diff('d'));
  }
}

template <class Sequence, class Pred>
Diffs CreateDiff(const Sequence& s1,
                 const Sequence& s2,
                 Pred pred)
{

  Diffs diffs;

  size_t n = s2.size();

  int start = 0;
  int m_end = s1.size() - 1;
  int n_end = s2.size() - 1;

  while(start <= m_end && start <= n_end && pred(s1[start], s2[start])) {
    diffs.push_back(Diff('m'));
    start++;
  }
  while(start <= m_end && start <= n_end && pred(s1[m_end], s2[n_end])) {
    m_end--;
    n_end--;
  }

  size_t m_new = m_end - start + 1;
  size_t n_new = n_end - start + 1;

  size_t** c = new size_t*[m_new + 1];
  for(size_t i = 0; i <= m_new; ++i) {
    c[i] = new size_t[n_new + 1];
    c[i][0] = 0;
  }
  for(size_t j = 0; j <= n_new; ++j)
    c[0][j] = 0;
  for(size_t i = 1; i <= m_new; ++i)
    for(size_t j = 1; j <= n_new; ++j)
      if(pred(s1[i - 1 + start], s2[j - 1 + start]))
        c[i][j] = c[i-1][j-1] + 1;
      else
        c[i][j] = c[i][j-1] > c[i-1][j] ? c[i][j-1] : c[i-1][j];

  CreateDiffRec(c, s1, s2, start, m_new, n_new, diffs, pred);

  for(size_t i = 0; i <= m_new; ++i)
    delete[] c[i];
  delete[] c;

  for (size_t i = n_end + 1; i < n; ++i)
    diffs.push_back(Diff('m'));

  return diffs;
}

template <class Sequence>
Diffs CreateDiff(const Sequence& s1, const Sequence& s2)
{
  return CreateDiff(s1, s2, std::equal_to<typename Sequence::value_type>());
}

template <class Sequence, class Sig, class Stats>
void AddStats(const Sequence& s1, const Sequence& s2, const Sig& sig, Stats& stats)
{
  if(sig.size() != stats.size())
    throw "Signature size differs from score array size.";

  size_t m = 0, d = 0, i = 0, s = 0;
  Diffs diff = CreateDiff(s1, s2);

  for(int j = 0; j < (int)diff.size(); ++j) {
    if(diff[j] == 'm')
      m++;
    else if(diff[j] == 'd') {
      d++;
      int k = 0;
      while(j - k >= 0 && j + 1 + k < (int)diff.size() &&
            diff[j - k] == 'd' && diff[j + 1 + k] == 'i') {
        d--;
        s++;
        k++;
      }
      j += k;
    } else if(diff[j] == 'i')
      i++;
  }

  for(size_t j = 0; j < sig.size(); ++j) {
    switch (sig[j]) {
    case 'l':
      stats[j] += d + i + s;
      break;
    case 'm':
      stats[j] += m;
      break;
    case 'd':
      stats[j] += d;
      break;
    case 'i':
      stats[j] += i;
      break;
    case 's':
      stats[j] += s;
      break;
    case 'r':
      float macc = 1;
      if (d + i + s + m)
        macc = 1.0 - (float)(d + i + s)/(float)(d + i + s + m);
      if(macc > 0)
        stats[j] += log(macc);
      else
        stats[j] += log(1.0/(float)(d + i + s + m + 1));
      break;
    }
  }
}

}

#endif
