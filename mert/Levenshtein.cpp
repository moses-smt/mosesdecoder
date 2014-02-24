#include <string>
#include <vector>
#include <iostream>
#include <functional>

using namespace std;

template <class Sequence, class BinaryPredicate>
size_t Levenshtein(const Sequence &s1, const Sequence &s2, BinaryPredicate pred)
{
  const size_t m(s1.size());
  const size_t n(s2.size());
 
  if(m == 0)
    return n;
  if(n == 0)
    return m;
 
  size_t *costs = new size_t[n + 1];
 
  for(size_t k = 0; k <= n; k++)
    costs[k] = k;
 
  size_t i = 0;
  for (typename Sequence::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1, ++i)
  {
    costs[0] = i + 1;
    size_t corner = i;
 
    size_t j = 0;
    for (typename Sequence::const_iterator it2 = s2.begin(); it2 != s2.end(); ++it2, ++j)
    {
      size_t upper = costs[j + 1];
      if(pred(*it1, *it2))
      {
	costs[j + 1] = corner;
      } else {
	size_t t(upper < corner ? upper : corner);
        costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
      } 
      corner = upper;
    }
  }
  
  size_t result = costs[n];
  delete [] costs;
  return result;
}

template <class Sequence>
size_t Levenshtein(const Sequence &s1, const Sequence &s2)
{
  return Levenshtein(s1, s2, std::equal_to<typename Sequence::value_type>());
}

 
int main()
{
	std::vector<std::string> v1;
	v1.push_back("This");
	v1.push_back("is");
	v1.push_back("a");
	v1.push_back("test");
	
	std::vector<std::string> v2;
	v2.push_back("This");
	v2.push_back("is");
	v2.push_back("not");
	v2.push_back("another");
	v2.push_back("test");
	
	cout << "distance : " 
	     << Levenshtein(v1, v2) << std::endl;
 
        return 0;
}
 