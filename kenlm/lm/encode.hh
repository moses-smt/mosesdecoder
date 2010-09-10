#ifndef LM_ENCODE_H__
#define LM_ENCODE_H__

#include <inttypes.h>

namespace lm {
namespace encode {

template <class Value> struct StructEncode {
  uint64_t GetKey() const { return key; }
  Value GetValue() const { return value; }

  void SetKey(uint64_t to) { key = to; }
  void SetValue(const Value &to) { value = to; }

  static size_t Size() { return sizeof(StructEncode<Value>); }
  static size_t Bits() { Size() * 8; }

  // Nominally private.  public to be a POD.  
  uint64_t key;
  Value value;
};

template <class Value> struct AppendEncode {
};

} // namespace encode
} // namespace lm

#endif // LM_ENCODE_H__
