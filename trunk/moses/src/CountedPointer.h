// $Id$

#ifndef COUNTEDPOINTER_H_
#define COUNTEDPOINTER_H_

// see http://ootips.org/yonat/4dev/counted_ptr.h
template <class T> class CountedPointer
{
public:
  explicit CountedPointer(T *p = 0) : pointerInfo_(0)
  {
    if (p)
      pointerInfo_ = new PointerAndCounter(p);
  }
  CountedPointer(const CountedPointer &p) { acquire(p.pointerInfo_); };
  ~CountedPointer() { release(); };
  CountedPointer &operator=(const CountedPointer &p)
  {
    if (this != &p) {
      release();
      acquire(p.pointerInfo_);
    }
    return *this;
  }
  CountedPointer &operator=(T *p)
  {
    release();
    pointerInfo_ = new PointerAndCounter(p);
    return *this;
  }

  operator bool() const { return pointerInfo_; };
  bool hasP() const { return pointerInfo_->pointer != 0; };

  T& operator*() const { return *pointerInfo_->pointer; };
  T* operator->() const { return pointerInfo_->pointer; };
  bool unique() const { return (!pointerInfo_ || pointerInfo_->counter == 1); };
  void destroy() { release(); };

  //void operator delete(void *p) { release(); };

private:
  struct PointerAndCounter
  {
    T *pointer;
    unsigned counter;
    PointerAndCounter(T* p = 0, unsigned c = 1) : pointer(p), counter(c) {};
  } *pointerInfo_;

  void acquire(PointerAndCounter *c)
  {
    pointerInfo_ = c;
    if (pointerInfo_)
      (pointerInfo_->counter)++;
  }

  void release()
  {
    if (pointerInfo_) {
      if (--(pointerInfo_->counter) == 0) {
        delete pointerInfo_->pointer;
        delete pointerInfo_;
      }
      pointerInfo_ = 0;
    }
  }
};
#endif
