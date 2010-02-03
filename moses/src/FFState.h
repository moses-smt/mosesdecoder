
#pragma once

#include <cassert>
#include <vector>

namespace Moses {

class FFState {
 public:
  virtual ~FFState();
  virtual int Compare(const FFState& other) const = 0;
};

class FFStateArray : public FFState {
private:
    std::vector<const FFState *> m_states;
    
public:
    FFStateArray() {}
    explicit FFStateArray(size_t nElements) : m_states(nElements) {}
    
    void push_back(const FFState *s) {
        m_states.push_back(s);
    }
    
    const FFState *operator[](size_t n) const {
        assert(n < m_states.size());
        return m_states[n];
    }
    
    const FFState *&operator[](size_t n) {
        assert(n < m_states.size());
        return m_states[n];
    }
    
    size_t size() const {
        return m_states.size();
    }
    
    virtual int Compare(const FFState& other) const {
	//return 0;
        if(&other == this)
            return 0;

        const FFStateArray *a = dynamic_cast<const FFStateArray *>(&other);
        
        // if the types are different, fall back on pointer comparison to get a well-defined ordering
        if(a == NULL)
            return &other < this ? -1 : 1;
        
        size_t i;
        for(i = 0; i < m_states.size(); i++) {
            if(i >= a->m_states.size())
                return -1;
            int comp = m_states[i]->Compare(*(a->m_states[i]));
            if(comp != 0)
                return comp;
        }
        
        if(i == a->m_states.size())
            return 0;
        else
            return 1;
    }
};
}
