#include "InputSource.h"

using namespace std;


namespace Josiah {

InputSource::~InputSource() {}

StreamInputSource::StreamInputSource(std::istream& is) : in(is) {
}

bool StreamInputSource::HasMore() const {
  return (in);
}

void StreamInputSource::GetSentence(std::string* sentence, int* lineno) {
  (void) lineno;
  std::getline(in, *sentence);
};


BatchedFileInputSource::BatchedFileInputSource(
            const string& filename, int rank, int size): m_next(0) {
   ifstream in(filename.c_str());
   if (!in) {
       throw runtime_error("Failed to open input file: " + filename);
   }

   vector<string> lines;
   string line;
   while(getline(in,line)) {
       lines.push_back(line);
   }
   float batchSize = (float)lines.size()/size;
   cerr << "Batch size: " << batchSize << endl;
   size_t start = (size_t)(rank*batchSize+0.5);
   size_t end = (size_t)((rank+1)*batchSize+0.5);
   m_lines.resize(end-start);
   copy(lines.begin()+start,lines.begin()+end,m_lines.begin());
   cerr << "batch start: " << start << " batch end: " << end << endl;

}

bool BatchedFileInputSource::HasMore() const {
    return m_next < m_lines.size();
}

void BatchedFileInputSource::GetSentence(string* sentence, int* lineno) {
    *lineno = m_next;
    *sentence = m_lines[m_next++];
}

}

