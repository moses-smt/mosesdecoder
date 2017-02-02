#pragma once

// generator/continuation for C++
// author: Andrew Fedoniouk @ terrainformatica.com
// idea borrowed from: "coroutines in C" Simon Tatham,
//                     http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
// BSD license

template<typename T>
  struct _generator
  {
    T* _stack;
    int _line;
    _generator():_stack(0), _line(-1) {}
    void _push() { T* n = new T; *n = *static_cast<T*>(this); _stack = n; }
    bool _pop() { if(!_stack) return false; T* t = _stack; *static_cast<T*>(this) = *_stack; t->_stack = 0; delete t; return true; }
    ~_generator() { while(_pop()); }
  };

  #define $generator(NAME) struct NAME : public _generator<NAME>

  #define $emit(T) bool operator()(T& _rv) { \
                      if(_line < 0) _line=0; \
                      $START: switch(_line) { case 0:;

  #define $stop  } _line = 0; if(_pop()) goto $START; return false; }

  #define $restart(WITH) { _push(); _stack->_line = __LINE__; _line=0; WITH; goto $START; case __LINE__:; }

  #define $yield(V)     \
          do {\
              _line=__LINE__;\
              _rv = (V); return true; case __LINE__:;\
          } while (0)
