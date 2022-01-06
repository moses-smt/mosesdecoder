#pragma once

#include <stack>
#include <vector>

namespace MosesTraining {
namespace Syntax {

template<typename T>
Tree<T>::~Tree() {
  for (typename std::vector<Tree *>::iterator p = children_.begin();
       p != children_.end(); ++p) {
    delete *p;
  }
}

template<typename T>
void Tree<T>::SetParents() {
  for (typename std::vector<Tree *>::iterator p = children_.begin();
       p != children_.end(); ++p) {
    (*p)->parent() = this;
    (*p)->SetParents();
  }
}

template<typename T>
std::size_t Tree<T>::Depth() const {
  std::size_t depth = 0;
  Tree *ancestor = parent_;
  while (ancestor != 0) {
    ++depth;
    ancestor = ancestor->parent_;
  }
  return depth;
}

template<typename T>
template<typename V>
class Tree<T>::PreOrderIter {
 public:
  PreOrderIter();
  PreOrderIter(V &);

  V &operator*() { return *node_; }
  V *operator->() { return node_; }

  PreOrderIter &operator++();
  PreOrderIter operator++(int);

  bool operator==(const PreOrderIter &);
  bool operator!=(const PreOrderIter &);

 private:
  // Pointer to the current node.
  V *node_;

  // Stack of indices defining the position of node_ within the child vectors
  // of its ancestors.
  std::stack<std::size_t> index_stack_;
};

template<typename T>
template<typename V>
Tree<T>::PreOrderIter<V>::PreOrderIter()
    : node_(0) {
}

template<typename T>
template<typename V>
Tree<T>::PreOrderIter<V>::PreOrderIter(V &t)
    : node_(&t) {
}

template<typename T>
template<typename V>
Tree<T>::PreOrderIter<V> &Tree<T>::PreOrderIter<V>::operator++() {
  // If the current node has children then visit the left-most child next.
  if (!node_->children().empty()) {
    index_stack_.push(0);
    node_ = node_->children()[0];
    return *this;
  }
  // Otherwise, try node's ancestors until either a node is found with a
  // sibling to the right or we reach the root (in which case the traversal
  // is complete).
  V *ancestor = node_->parent_;
  while (ancestor) {
    std::size_t index = index_stack_.top();
    index_stack_.pop();
    if (index+1 < ancestor->children_.size()) {
      index_stack_.push(index+1);
      node_ = ancestor->children()[index+1];
      return *this;
    }
    ancestor = ancestor->parent_;
  }
  node_ = 0;
  return *this;
}

template<typename T>
template<typename V>
Tree<T>::PreOrderIter<V> Tree<T>::PreOrderIter<V>::operator++(int) {
  PreOrderIter tmp(*this);
  ++*this;
  return tmp;
}

template<typename T>
template<typename V>
bool Tree<T>::PreOrderIter<V>::operator==(const PreOrderIter &rhs) {
  return node_ == rhs.node_;
}

template<typename T>
template<typename V>
bool Tree<T>::PreOrderIter<V>::operator!=(const PreOrderIter &rhs) {
  return node_ != rhs.node_;
}

template<typename T>
template<typename V>
class Tree<T>::LeafIter {
 public:
  LeafIter();
  LeafIter(V &);

  V &operator*() { return *node_; }
  V *operator->() { return node_; }

  LeafIter &operator++();
  LeafIter operator++(int);

  bool operator==(const LeafIter &);
  bool operator!=(const LeafIter &);

 private:
  // Pointer to the current node.
  V *node_;

  // Stack of indices defining the position of node_ within the child vectors
  // of its ancestors.
  std::stack<std::size_t> index_stack_;
};

template<typename T>
template<typename V>
Tree<T>::LeafIter<V>::LeafIter()
    : node_(0) {
}

template<typename T>
template<typename V>
Tree<T>::LeafIter<V>::LeafIter(V &t)
    : node_(&t) {
  // Navigate to the first leaf.
  while (!node_->IsLeaf()) {
    index_stack_.push(0);
    node_ = node_->children()[0];
  }
}

template<typename T>
template<typename V>
Tree<T>::LeafIter<V> &Tree<T>::LeafIter<V>::operator++() {
  // Try node's ancestors until either a node is found with a sibling to the
  // right or we reach the root (in which case the traversal is complete).
  V *ancestor = node_->parent_;
  while (ancestor) {
    std::size_t index = index_stack_.top();
    index_stack_.pop();
    if (index+1 < ancestor->children_.size()) {
      index_stack_.push(index+1);
      node_ = ancestor->children()[index+1];
      // Navigate to the first leaf.
      while (!node_->IsLeaf()) {
        index_stack_.push(0);
        node_ = node_->children()[0];
      }
      return *this;
    }
    ancestor = ancestor->parent_;
  }
  node_ = 0;
  return *this;
}

template<typename T>
template<typename V>
Tree<T>::LeafIter<V> Tree<T>::LeafIter<V>::operator++(int) {
  LeafIter tmp(*this);
  ++*this;
  return tmp;
}

template<typename T>
template<typename V>
bool Tree<T>::LeafIter<V>::operator==(const LeafIter &rhs) {
  return node_ == rhs.node_;
}

template<typename T>
template<typename V>
bool Tree<T>::LeafIter<V>::operator!=(const LeafIter &rhs) {
  return node_ != rhs.node_;
}

}  // namespace Syntax
}  // namespace MosesTraining
