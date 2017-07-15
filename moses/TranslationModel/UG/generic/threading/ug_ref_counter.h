#include "ug_thread_safe_counter.h"
#pragma once
// obsolete once intrusive_ref_counter is available everywhere

namespace Moses {

  class reference_counter
  {
  public:
    friend void intrusive_ptr_add_ref(reference_counter const* p)
    {
      if (p) ++p->m_refcount;
    }
    friend void intrusive_ptr_release(reference_counter const* p)
    {
      if (p && --p->m_refcount == 0) 
	delete p;
    }
  protected:
    reference_counter() {}
    virtual ~reference_counter() {};
  private:
    mutable ThreadSafeCounter m_refcount;
  };
}
