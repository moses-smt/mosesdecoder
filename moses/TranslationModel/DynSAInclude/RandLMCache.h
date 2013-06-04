// Copyright 2008 Abby Levenberg, David Talbot
//
// This file is part of RandLM
//
// RandLM is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RandLM is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RandLM.  If not, see <http://www.gnu.org/licenses/>.
#ifndef INC_RANDLM_CACHE_H
#define INC_RANDLM_CACHE_H

#include <iterator>
#include <map>
#include <ctime>
#include <iostream>

namespace randlm
{

//! @todo ask abby2
template<typename T>
class CacheNode
{
public:
  typedef std::map<wordID_t, CacheNode<T>* > childMap;
  // initialise value to 'unknown' (i.e. not yet queried or cached).
  CacheNode(T unknown_value) : value_(unknown_value) {}
  childMap childs_;  // child pointers
  T value_;  // value stored
  const void* state_;  // state pointer
};

template<typename T>
class Cache
{
public:
  typedef typename std::map<wordID_t, CacheNode<T>* >::iterator childPtr;
  // unknown_value is used to indicate the ngram was not queried (yet)
  // null_value_ indicates it was queried but not found in model
  // space usage is handled by client.
  Cache(T unknown_value, T null_value)  :
    cur_nodes_(0), unknown_value_(unknown_value), null_value_(null_value) {
    root_ = newNode();
  }
  ~Cache() {
    if(clear()) {
      delete root_;
      root_ = NULL;
    }  else {
      std::cerr << "Error freeing cache memory.\n";
    }
  }
  bool setCacheNgram(const wordID_t* ngram, int len, T value, const void* state) {
    // inserts full ngram into cache
    CacheNode<T>* node = root_;
    for (int i = len - 1; i > -1; --i) {
      childPtr child = node->childs_.find(ngram[i]);
      if( child != node->childs_.end() ) {
        // current node is already prefix. Go to child node
        node = node->childs_[ngram[i]];
      }  else {
        // no child for prefix. set new child link in current node
        CacheNode<T> * newChild = newNode(node);
        node->childs_[ngram[i]] = newChild;
        // go to new node
        node = newChild;
      }
    }
    node->value_ = value;
    node->state_ = state;
    return true;
  }
  bool checkCacheNgram(const wordID_t* ngram, int len, T* value, const void** state) {
    // finds value for this full ngram only (returns false if full ngram not in cache)
    CacheNode<T> * node = root_;
    for(int i = len - 1; i > -1; --i) {
      // go to deepest level node of ngram in cache
      childPtr child = node->childs_.find(ngram[i]);
      if( child != node->childs_.end() ) {
        // switch to child node
        node = node->childs_[ngram[i]];
      } else {
        // not cached
        return false;
      }
    }
    *value = node->value_;
    if(state) *state = node->state_;
    return *value != null_value_ && *value != unknown_value_;
  }
  int getCache2(const wordID_t* ngram, int len, T** values, int* found) {
    // set values array to point to cache value nodes
    CacheNode<T> * node = root_;
    *found = 0;
    //values[0] = &node->value_; // pointer to root node's value
    bool all_found = true;
    for(int i = len - 1; i > -1; --i) {
      // go to deepest level node of ngram in cache
      childPtr child = node->childs_.find(ngram[i]);
      if( child != node->childs_.end() ) {
        // switch to child node
        node = node->childs_[ngram[i]];
        // get pointer to value (index by length - 1)
        values[i] = &node->value_;
        // if null_value then assume all extensions impossible
        if (node->value_ == null_value_) {
          return len - 1 - i; // max length posible
        }
        all_found = all_found && (node->value_ != unknown_value_);
        if (all_found)
          ++(*found);
      } else {
        // initialise uncached values
        CacheNode<T> * newChild = newNode(node);
        node->childs_[ngram[i]] = newChild;
        // go to new node
        node = newChild;
        values[i] = &node->value_;
      }
    }
    return len; // all possible
  }
  int getCache(const wordID_t* ngram, int len, T** values, int* found) {
    // get pointers to values for ngram and constituents.
    // returns upper bound on longest subngram in model.
    // 'found' stores longest non-null and known value found.
    CacheNode<T> * node = root_;
    *found = 0;
    values[0] = &node->value_; // pointer to root node's value
    bool all_found = true;
    for(int i = len - 1; i > -1; --i) {
      // go to deepest level node of ngram in cache
      childPtr child = node->childs_.find(ngram[i]);
      if( child != node->childs_.end() ) {
        // switch to child node
        node = node->childs_[ngram[i]];
        // get pointer to value (index by length - 1)
        values[len - i] = &node->value_;
        // if null_value then assume all extensions impossible
        if (node->value_ == null_value_)
          return len - 1 - i; // max length posible
        all_found = all_found && (node->value_ != unknown_value_);
        if (all_found)
          ++(*found);
      } else {
        // initialise uncached values
        CacheNode<T> * newChild = newNode(node);
        node->childs_[ngram[i]] = newChild;
        // go to new node
        node = newChild;
        values[len - i] = &node->value_;
      }
    }
    return len; // all possible
  }
  bool clear() {
    std::cerr << "Clearing cache with " << static_cast<float>(cur_nodes_ * nodeSize())
              / static_cast<float>(1ull << 20) << "MB" << std::endl;
    return clearNodes(root_);
  }
  int nodes() {
    // returns number of nodes
    return cur_nodes_;
  }
  int nodeSize() {
    return sizeof(CacheNode<T>) + sizeof(root_->childs_);
  }
private:
  CacheNode<T> * root_;
  count_t cur_nodes_;
  T unknown_value_; // Used to initialise data at each node
  T null_value_; // Indicates cached something not in model
  CacheNode<T>* newNode(CacheNode<T> * node = 0) {
    ++cur_nodes_;
    return new CacheNode<T>(unknown_value_);
  }
  bool clearNodes(CacheNode<T> * node) {
    //delete children from this node
    if(!node->childs_.empty()) {
      iterate(node->childs_, itr) {
        if(!clearNodes(itr->second))
          std::cerr << "Error emptying cache\n";
        delete itr->second;
        --cur_nodes_;
      }
      node->childs_.clear();
    }
    return true;
  }

};
} //end namespace
#endif //INC_RANDLM_CACHE_H
