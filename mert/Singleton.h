#ifndef MERT_SINGLETON_H_
#define MERT_SINGLETON_H_

#include <cstdlib>

namespace MosesTuning
{


// thread *un*safe singleton.
// TODO: replace this with thread-safe singleton.
template <typename T>
class Singleton
{
public:
  static T* GetInstance() {
    if (m_instance == NULL) {
      m_instance = new T;
    }
    return m_instance;
  }

  static void Delete() {
    if (m_instance) {
      delete m_instance;
      m_instance = NULL;
    }
  }

private:
  Singleton();
  static T* m_instance;
};

template <typename T>
T* Singleton<T>::m_instance = NULL;

}

#endif  // MERT_SINGLETON_H_
