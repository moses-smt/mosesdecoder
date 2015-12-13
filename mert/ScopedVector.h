#ifndef MERT_SCOPED_VECTOR_H_
#define MERT_SCOPED_VECTOR_H_

#include <vector>

namespace MosesTuning
{

template <class T>
class ScopedVector
{
public:
  typedef typename std::vector<T*>::iterator iterator;
  typedef typename std::vector<T*>::const_iterator const_iterator;

  ScopedVector() {}
  virtual ~ScopedVector() {
    reset();
  }

  bool empty() const {
    return m_vec.empty();
  }

  void push_back(T *e) {
    m_vec.push_back(e);
  }

  void reset() {
    for (iterator it = m_vec.begin(); it != m_vec.end(); ++it) {
      delete *it;
    }
    m_vec.clear();
  }

  void reserve(std::size_t capacity) {
    m_vec.reserve(capacity);
  }
  void resize(std::size_t size) {
    m_vec.resize(size);
  }

  std::size_t size() const {
    return m_vec.size();
  }

  iterator begin() {
    return m_vec.begin();
  }
  const_iterator begin() const {
    return m_vec.begin();
  }

  iterator end() {
    return m_vec.end();
  }
  const_iterator end() const {
    return m_vec.end();
  }

  std::vector<T*>& get() {
    return m_vec;
  }
  const std::vector<T*>& get() const {
    return m_vec;
  }

  std::vector<T*>* operator->() {
    return &m_vec;
  }
  const std::vector<T*>* operator->() const {
    return &m_vec;
  }

  T*& operator[](std::size_t i) {
    return m_vec[i];
  }
  const T* operator[](std::size_t i) const {
    return m_vec[i];
  }

private:
  std::vector<T*> m_vec;

  // no copying allowed.
  ScopedVector<T>(const ScopedVector<T>&);
  void operator=(const ScopedVector<T>&);
};

}

#endif // MERT_SCOPED_VECTOR_H_
