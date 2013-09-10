// This program takes gzipped sorted files and merges them in sorted order
// to stdout. Written by Ulrich Germann
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include <string>
#include <vector>
#include "../../../moses/generic/file_io/ug_stream.h"
using namespace std;
using namespace ugdiss;
using namespace boost::iostreams;

typedef boost::shared_ptr<filtering_istream> fptr;

class Part
{
  string       fname;
  fptr             f;
  string my_lines[2];
  size_t         ctr;
public:
  string const& line() const 
  { 
    static string empty_line;
    return f ? my_lines[ctr%2] : empty_line; 
  }

  Part(string _fname) : ctr(0)
  {
    fname = _fname;
    f.reset(open_input_stream(fname));
    if (!getline(*f, my_lines[0])) f.reset();
  }

  bool next() 
  {
    if (!f) return false;
    if (!getline(*f, my_lines[++ctr%2]))
      {
        f.reset();
        --ctr;
        return false;
      }
     assert(my_lines[(ctr-1)%2] <= my_lines[ctr%2]);
    return true;
  }

  bool operator <(Part const& other) const 
  { return line() < other.line(); }

  bool operator <=(Part const& other) const 
  { return line() <= other.line(); }

  bool operator >(Part const& other) const 
  { return line() > other.line(); }

  bool operator >=(Part const& other) const 
  { return line() >= other.line(); }

  bool go(ostream& out)
  {
    if (!f) return false;
#if 0
    if (ctr)
      {
        out << fname << "-" << ctr - 1 << "-";
        out << my_lines[(ctr - 1)%2] << endl;
      }
    do 
      {
        out << fname << " " << ctr << " ";
        out << line() << "\n";
      }
    while (next() && my_lines[0] == my_lines[1]);
#else
    do    { out << line() << "\n"; } 
    while (next() && my_lines[0] == my_lines[1]);
    out.flush();
#endif
    return f != NULL;
  }
  
};


int main(int argc, char* argv[])
{
  vector<Part> parts;
  for (int i = 1; i < argc; ++i)
    parts.push_back(Part(argv[i]));
  make_heap(parts.begin(), parts.end(), greater<Part>());
  while (parts.size())
    {
      pop_heap(parts.begin(), parts.end(), greater<Part>());
      if (parts.back().go(cout))
        push_heap(parts.begin(), parts.end(), greater<Part>());
      else parts.pop_back();
    }
}
