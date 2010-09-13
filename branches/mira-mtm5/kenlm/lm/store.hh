#ifndef LM_STORE_H__
#define LM_STORE_H__

namespace lm {

template <class Key, class Value, class Encoding, class Ptr, size_t Multiply> class SimpleIterator {
  private:
    typedef SimpleIterator<Key, Value, Encoding, Ptr, Multiply> S;
  public:
    SimpleIterator() {}
    explicit SimpleIterator(const char *begin) : ptr_(reinterpret_cast<Ptr>(begin)) {}
    explicit SimpleIterator(char *begin) : ptr_(reinterpret_cast<Ptr>(begin)) {}

    bool operator==(const S &other) { return ptr_ == other.ptr_; }

    S &operator++() {
      ptr_ += Multiply;
      return *this;
    }
    S &operator+=(size_t amount) {
      ptr_ += Multiply * amount;
      return *this;
    }
    S &operator--() {
      ptr_ -= Multiply;
      return *this;
    }
    S &operator-=(size_t amount) {
      ptr_ -= Multiply * amount;
      return *this;
    }

    size_t operator-(const S &other) {
      return (ptr_ - other.ptr_) / Multiply;
    }

    Key GetKey() { return Entry::GetKey(ptr_); }
    Value GetValue() { return Entry::GetValue(ptr_); }
    // These shouldn't be a problem due to template magic.  
    void SetKey(Key to) { Entry::SetKey(ptr_, to); }
    void SetValue(const Value &to) { Entry::SetValue(ptr_, to); }

  protected:
    Ptr ptr_;
};

template <class Key, class Value> class AlignedArray {
  public:
    static const size_t kBytes = sizeof(Entry);
    static const size_t kBits = kBytes * 8;

    typedef SimpleIterator<Key, Value, Encoding, const Entry*, 1> ConstIterator;
    typedef SimpleIterator<Key, Value, Encoding, Entry*, 1> MutableIterator;

  private:
    struct Entry {
      Key key;
      Value value;
    };

    struct Encoding {
      static Key GetKey(const Entry *e) { return e->key; }
      static Value GetValue(const Entry *e) { return e->value; }
      static void SetKey(Entry *e, Key key) { e->key = key; }
      static void SetValue(Entry *e, const Value &value) { e->value = value; }
    };
};

template <class Key, class Value> class ByteAlignedArray {
  public:
    static const size_t kBytes = sizeof(Key) + sizeof(Value);
    static const size_t kBits = kBytes * 8;

    typedef SimpleIterator<Key, Value, Encoding, const char *, kBytes> ConstIterator;
    typedef SimpleIterator<Key, Value, Encoding, char*, kBytes> MutableIterator;

  private:
    struct Encoding {
      static Key GetKey(const char *a) {
        return *reinterpret_cast<const Key *>(a);
      }
      static Value GetValue(const char *a) {
        return *reinterpret_cast<const Value*>(a + sizeof(Key));
      }
      static void SetKey(char *a, Key key) {
        *reinterpret_cast<Key *>(a) = key;
      }
      static void SetValue(char *a, const Value &value) {
        *reinterpret_cast<Value*>(a + sizeof(Key)) = value;
      }
    };
};

template <class Key, class Value> class AlternatingArray {
  public:
    static const size_t kBytes = sizeof(Entry) / 2;
    static const size_t kBits = sizeof(Entry) * 4;

    typedef SimpleIterator<Key, Value, Encoding, size_t, 1> ConstIterator;
    typedef SimpleIterator<Key, Value, Encoding, size_t, 1> MutableIterator;

  private:
    // Here's hoping the % operations compile to bit operations.  
    struct Encoding {
      static Key GetKey(const char *a) {

        const Entry &ent = *reinterpret_cast<const Entry *>(a);
        return (reinterpret_cast<std::size_t>(a) % sizeof(Entry)) ? ent.key0 : ent.key1;
      }
      static Value GetValue(const char *a) {
        const Entry &val = *reinterpret_cast<const Entry *>(a);
        return (a % sizeof(Entry)) ? ent.value0 : ent.value1;
      }
      static void SetKey(char *a, Key key) {
        Entry &val = *reinterpret_cast<Entry *>(a);
        ((a % sizeof(Entry)) ? ent.key0 : ent.key1) = key;
      }
      static void SetValue(char *a, const Value &value) {
        Entry &val = *reinterpret_cast<Entry *>(a);
        ((a % sizeof(Entry)) ? ent.value0 : ent.value1) = value;
      }
    };
};

} // namespace lm

#endif // LM_STORE_H__
