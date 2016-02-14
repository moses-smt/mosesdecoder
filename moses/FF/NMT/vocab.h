#pragma once

#include <map>
#include <string>
#include <vector>
#include <fstream>

class Vocab {
  public:
    Vocab(const std::string& txt) {
        std::ifstream in(txt.c_str());
        size_t c = 0;
        std::string line;
        while(std::getline(in, line)) {
            str2id_[line] = c++;
            id2str_.push_back(line);
        }
        str2id_["</s>"] = c;
        id2str_.push_back("</s>");
    }

    size_t operator[](const std::string& word) const {
        auto it = str2id_.find(word);
        if(it != str2id_.end())
            return it->second;
        else
            return 1;
    }

    inline std::vector<size_t> Encode(const std::vector<std::string> sentence) {
      std::vector<size_t> indexes;
      for (auto& word : sentence) {
        indexes.push_back((*this)[word]);
      }
      return indexes;
    }


    const std::string& operator[](size_t id) const {
        return id2str_[id];
    }

    size_t size() {
      return id2str_.size();
    }

  private:
    std::map<std::string, size_t> str2id_;
    std::vector<std::string> id2str_;
};
