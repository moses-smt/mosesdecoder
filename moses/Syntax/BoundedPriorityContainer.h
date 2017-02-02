#pragma once

#include <queue>
#include <vector>

namespace Moses
{
namespace Syntax
{

// A container that can hold up to k objects of type T, each with an associated
// priority.  The container accepts new elements unconditionally until the
// limit is reached.  After that, elements are only accepted if they have a
// higher priority than the worst element (which they displace).
//
// BoundedPriorityContainer does not preserve the insertion order of the
// elements (or provide any other guarantees about order).
//
// BoundedPriorityContainer pre-allocates space for all k objects.
//
// (Although BoundedPriorityContainer is implemented using a priority queue,
// it doesn't provide the interface of a priority queue, hence the generic
// name 'container'.)
template<typename T>
class BoundedPriorityContainer
{
public:
  typedef typename std::vector<T>::iterator Iterator;
  typedef typename std::vector<T>::const_iterator ConstIterator;

  BoundedPriorityContainer(std::size_t);

  Iterator Begin() {
    return m_elements.begin();
  }
  Iterator End() {
    return m_elements.begin()+m_size;
  }

  ConstIterator Begin() const {
    return m_elements.begin();
  }
  ConstIterator End() const {
    return m_elements.begin()+m_size;
  }

  // Return the number of elements currently held.
  std::size_t Size() const {
    return m_size;
  }

  // 'Lazily' clear the container by setting the size to 0 (allowing elements
  // to be overwritten).
  // TODO Eliminate heap-reorganisation overhead by using a vector-based heap
  // TODO directly instead of priority_queue, which requires pop() to clear
  // TODO Alternative, is to clear m_queue by assigning an empty queue value
  // TODO but that might incur an alloc-related overhead when the new underlying
  // TODO has to be regrown.
  void LazyClear() {
    m_size = 0;
    while (!m_queue.empty()) {
      m_queue.pop();
    }
  }

  // Insert the given object iff
  //   i) the container is not full yet, or
  //  ii) the new object has a higher priority than the worst one already
  //      stored.
  // The return value specifies whether or not the element was inserted.
  bool Insert(const T &, float);

  // Insert the given object iff
  //   i) the container is not full yet, or
  //  ii) the new object has a higher priority than the worst one already
  //      stored.
  // If the element is inserted then, for efficiency reasons, it is swapped in
  // rather than copied.  This requires that T provides a swap() function.  The
  // return value specifies whether or not the element was inserted.
  // TODO Test if this is actually any faster than Insert() in practice.
  bool SwapIn(T &, float);

  // Determine if an object with the given priority would be accepted for
  // insertion based on the current contents of the container.
  bool WouldAccept(float priority) {
    return m_size < m_limit || priority > m_queue.top().first;
  }

private:
  typedef std::pair<float, int> PriorityIndexPair;

  class PriorityIndexPairOrderer
  {
  public:
    bool operator()(const PriorityIndexPair &p,
                    const PriorityIndexPair &q) const {
      return p.first > q.first;
    }
  };

  // Min-priority queue.  The queue stores the indices of the elements, not
  // the elements themselves to keep down the costs of heap maintenance.
  typedef std::priority_queue<PriorityIndexPair,
          std::vector<PriorityIndexPair>,
          PriorityIndexPairOrderer> Queue;

  // The elements are stored in a vector.  Note that the size of this vector
  // can be greater than m_size (after a call to LazyClear).
  std::vector<T> m_elements;

  // The number of elements currently held.
  std::size_t m_size;

  // The maximum number of elements.
  const std::size_t m_limit;

  // The min-priority queue.
  Queue m_queue;
};

template<typename T>
BoundedPriorityContainer<T>::BoundedPriorityContainer(std::size_t limit)
  : m_size(0)
  , m_limit(limit)
{
  m_elements.reserve(m_limit);
}

template<typename T>
bool BoundedPriorityContainer<T>::Insert(const T &t, float priority)
{
  if (m_size < m_limit) {
    PriorityIndexPair pair(priority, m_size);
    m_queue.push(pair);
    if (m_size < m_elements.size()) {
      m_elements[m_size] = t;
    } else {
      m_elements.push_back(t);
    }
    ++m_size;
    return true;
  } else if (priority > m_queue.top().first) {
    PriorityIndexPair pair = m_queue.top();
    m_queue.pop();
    pair.first = priority;
    m_elements[pair.second] = t;
    m_queue.push(pair);
    return true;
  }
  return false;
}

template<typename T>
bool BoundedPriorityContainer<T>::SwapIn(T &t, float priority)
{
  if (m_size < m_limit) {
    PriorityIndexPair pair(priority, m_size);
    m_queue.push(pair);
    if (m_size < m_elements.size()) {
      swap(m_elements[m_size], t);
    } else {
      m_elements.push_back(t);
    }
    ++m_size;
    return true;
  } else if (priority > m_queue.top().first) {
    PriorityIndexPair pair = m_queue.top();
    m_queue.pop();
    pair.first = priority;
    swap(m_elements[pair.second], t);
    m_queue.push(pair);
    return true;
  }
  return false;
}

}  // Syntax
}  // Moses
