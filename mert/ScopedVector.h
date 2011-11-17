#ifndef SCOPEDVECTOR_H_
#define SCOPEDVECTOR_H_

#include <vector>

template <class T>
class ScopedVector {
 public:
  typedef typename std::vector<T*>::iterator iterator;
  typedef typename std::vector<T*>::const_iterator const_iterator;

  ScopedVector() {}
  virtual ~ScopedVector() { reset(); }

  bool empty() const { return vec_.empty(); }

  void push_back(T *e) { vec_.push_back(e); }

  void reset() {
    for (iterator it = vec_.begin(); it != vec_.end(); ++it) {
      delete *it;
    }
    vec_.clear();
  }

  void reserve(size_t capacity) { vec_.reserve(capacity); }
  void resize(size_t size) { vec_.resize(size); }

  size_t size() const {return vec_.size(); }

  iterator begin() { return vec_.begin(); }
  const_iterator begin() const { return vec_.begin(); }

  iterator end() { return vec_.end(); }
  const_iterator end() const { return vec_.end(); }

  std::vector<T*>& get() { return vec_; }
  const std::vector<T*>& get() const { return vec_; }

  std::vector<T*>* operator->() { return &vec_; }
  const std::vector<T*>* operator->() const { return &vec_; }

  T*& operator[](size_t i) { return vec_[i]; }
  const T* operator[](size_t i) const { return vec_[i]; }

 private:
  std::vector<T*> vec_;

  // no copying allowed.
  ScopedVector<T>(const ScopedVector<T>&);
  void operator=(const ScopedVector<T>&);
};

#endif // SCOPEDVECTOR_H_
