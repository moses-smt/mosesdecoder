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
class Tree<T>::PreOrderIterator {
 public:
  PreOrderIterator();
  PreOrderIterator(Tree<T> &);

  Tree<T> &operator*() { return *node_; }
  Tree<T> *operator->() { return node_; }

  PreOrderIterator &operator++();
  PreOrderIterator operator++(int);

  bool operator==(const Tree<T>::PreOrderIterator &);
  bool operator!=(const Tree<T>::PreOrderIterator &);

 private:
  // Pointer to the current node.
  Tree<T> *node_;

  // Stack of indices defining the position of node_ within the child vectors
  // of its ancestors.
  std::stack<std::size_t> index_stack_;
};

template<typename T>
Tree<T>::PreOrderIterator::PreOrderIterator()
    : node_(0) {
}

template<typename T>
Tree<T>::PreOrderIterator::PreOrderIterator(Tree<T> &t)
    : node_(&t) {
}

template<typename T>
typename Tree<T>::PreOrderIterator &Tree<T>::PreOrderIterator::operator++() {
  // If the current node has children then visit the left-most child next.
  if (!node_->children().empty()) {
    index_stack_.push(0);
    node_ = node_->children()[0];
    return *this;
  }
  // Otherwise, try node's ancestors until either a node is found with a
  // sibling to the right or we reach the root (in which case the traversal
  // is complete).
  Tree<T> *ancestor = node_->parent_;
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
typename Tree<T>::PreOrderIterator Tree<T>::PreOrderIterator::operator++(int) {
  PreOrderIterator tmp(*this);
  ++*this;
  return tmp;
}

template<typename T>
bool Tree<T>::PreOrderIterator::operator==(const PreOrderIterator &rhs) {
  return node_ == rhs.node_;
}

template<typename T>
bool Tree<T>::PreOrderIterator::operator!=(const PreOrderIterator &rhs) {
  return node_ != rhs.node_;
}

}  // namespace Syntax
}  // namespace MosesTraining
