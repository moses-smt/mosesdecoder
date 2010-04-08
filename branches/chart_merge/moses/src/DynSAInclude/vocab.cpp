#include <sstream>
#include "vocab.h"

namespace Moses {

	// Vocab class
	const wordID_t Vocab::kOOVWordID;
	const wordID_t Vocab::kBOSWordID;	
	const word_t Vocab::kBOS = "<s>";
	const word_t Vocab::kEOS = "</s>";
	const word_t Vocab::kOOVWord = "<unk>";
		
	wordID_t Vocab::getWordID(const word_t& word) {
		// get id and possibly add to vocab 
		if (words2ids_.find(word) == words2ids_.end())
			if (!closed_) {
				wordID_t id = words2ids_.size() + 1;
				words2ids_[word] = id; // size() returns size AFTER insertion of word
				ids2words_[id] = word; // so size() is the same here ... 
			} 
			else {
				return Vocab::kOOVWordID;
			}
		wordID_t id = words2ids_[word];
		return id;
	}
		
	word_t Vocab::getWord(wordID_t id) {
		// get word string given id
		return (ids2words_.find(id) == ids2words_.end()) ? Vocab::kOOVWord : ids2words_[id];
	}
	
	bool Vocab::inVocab(wordID_t id) {
		return ids2words_.find(id) != ids2words_.end();
	}

	bool Vocab::inVocab(const word_t & word) {
		return words2ids_.find(word) != words2ids_.end();
	}
	
	bool Vocab::save(const std::string & vocab_path) {
		// save vocab as id -> word 
		FileHandler vcbout(vocab_path, std::ios::out);
		return save(&vcbout);
	}
	bool Vocab::save(FileHandler* vcbout) {
		// then each vcb entry
		*vcbout << ids2words_.size() << "\n";
		iterate(ids2words_, iter)
			*vcbout << iter->second << "\t" << iter->first << "\n";
		return true;
	}
	
	bool Vocab::load(const std::string & vocab_path, bool closed) {
		FileHandler vcbin(vocab_path, std::ios::in);
		std::cerr << "Loading vocab from " << vocab_path << std::endl;
		return load(&vcbin, closed);
	}
	bool Vocab::load(FileHandler* vcbin, bool closed) {
		// load vocab id -> word mapping 
		words2ids_.clear();	// reset mapping
		ids2words_.clear();
		std::string line;
		word_t word;
		wordID_t id;
		assert(getline(*vcbin, line));
		std::istringstream first(line.c_str());
		uint32_t vcbsize(0);
		first >> vcbsize;
		uint32_t loadedsize = 0;
		while (loadedsize++ < vcbsize && getline(*vcbin, line)) {
			std::istringstream entry(line.c_str());
			entry >> word;
			entry >> id; 
			// may be no id (i.e. file may just be a word list)
			if (id == 0 && word != Vocab::kOOVWord) 
				id = ids2words_.size() + 1;	// assign ids sequentially starting from 1
			assert(ids2words_.count(id) == 0 && words2ids_.count(word) == 0);
			ids2words_[id] = word;
			words2ids_[word] = id;
		}
		closed_ = closed;	// once loaded fix vocab ?
		std::cerr << "Loaded vocab with " << ids2words_.size() << " words." << std::endl;
		return true;
	}
	void Vocab::printVocab() {
		iterate(ids2words_, iter)
			std::cerr << iter->second << "\t" << iter->first << "\n";
		iterate(words2ids_, iter)
			std::cerr << iter->second << "\t" << iter->first << "\n";
	}

} //end namespace
