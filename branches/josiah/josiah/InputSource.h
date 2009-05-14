#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

namespace Josiah {

struct InputSource {
  virtual bool HasMore() const = 0;
  virtual void GetSentence(std::string* sentence, int* lineno) = 0;
  virtual ~InputSource();
};

struct StreamInputSource : public InputSource {
  std::istream& in;
  StreamInputSource(std::istream& is);
  virtual bool HasMore() const;
  virtual void GetSentence(std::string* sentence, int* lineno);
};


/**
 * Splits a file into batches.
 **/
class BatchedFileInputSource : public InputSource {
    public:
        BatchedFileInputSource(
            const std::string& filename, int rank, int size);

        virtual bool HasMore() const;
        virtual void GetSentence(std::string* sentence, int* lineno);

    private:
        std::vector<std::string> m_lines;
        size_t m_next;
};

}

