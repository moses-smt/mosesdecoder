/**
 * ISS (Indexed Strings Storage) - memory efficient storage for permanent strings.
 *
 * Implementation note: use #define USE_HASHSET to switch between implementation
 * using __gnu_cxx::hash_set and implementation using std::set.
 *
 * (C) Ceslav Przywara, UFAL MFF UK, 2011
 *
 * $Id$
 */

#ifndef _ISS_H
#define	_ISS_H

#include <limits>
#include <vector>
#include <string.h>

// Use hashset instead of std::set for string-to-number indexing?
#ifdef USE_HASHSET
#include <ext/hash_set>
#else
#include <set>
#endif

#include <boost/pool/pool.hpp>

#ifdef USE_HASHSET
// Forward declaration of comparator functor.
template<class IndType>
class StringsEqualComparator;

template<class IndType>
class Hasher;
#else
// Forward declaration of comparator functor.
template<class IndType>
class StringsLessComparator;
#endif

/**
 */
template<class IndType>
class IndexedStringsStorage {

public:

    typedef IndType index_type;

#ifdef USE_HASHSET
    typedef StringsEqualComparator<IndType> equality_comparator_t;

    typedef Hasher<IndType> hasher_t;

    /** @typedef Hash set used as lookup table (string -> numeric index). */
    typedef __gnu_cxx::hash_set<IndType, hasher_t, equality_comparator_t> index_t;
#else
    typedef StringsLessComparator<IndType> less_comparator_t;

    /** @typedef Set used as lookup table (string -> numeric index). */
    typedef std::set<IndType, less_comparator_t> index_t;
#endif
    /** @typedef Container of pointers to stored C-strings. Acts as
     *  conversion table: numeric index -> string.
     */
    typedef std::vector<const char*> table_t;

private:

    /** @var memory pool used to store C-strings */
    boost::pool<> _storage;

    /** @var index-to-string conversion table */
    table_t _table;

    /** @var index lookup table */
    index_t _index;

public:
    /** Default constructor.
     */
    IndexedStringsStorage(void);

    /** @return True, if the indices are exhausted (new strings cannot be stored).
     */
    inline bool is_full(void) const { return _table.size() == std::numeric_limits<IndType>::max(); }

    /** Retrieves pointer to C-string instance represented by given index.
     * Note: No range checks are performed!
     * @param index Index of C-string to retrieve.
     * @return Pointer to stored C-string instance.
     */
    inline const char* get(IndType index) const { return _table[index]; }

    /** Stores the string and returns its numeric index.
     * @param str Pointer to C-string to store.
     * @return Index of stored copy of str.
     * @throw std::bad_alloc When insertion of new string would cause
     *		overflow of indices datatype.
     */
    IndType put(const char* str);

    /** @return Number of unique strings stored so far.
     */
    inline table_t::size_type size(void) const { return _table.size(); }
};


/** Functor designed for less than comparison of C-strings stored within StringStore.
 * @param IndType Type of numerical indices of strings within given StringStore.
 */
#ifdef USE_HASHSET
template<class IndType>
class StringsEqualComparator: public std::binary_function<IndType, IndType, bool> {
#else
template<class IndType>
class StringsLessComparator: public std::binary_function<IndType, IndType, bool> {
#endif
    /** @var conversion table: index -> string (necessary for indices comparison) */
    const typename IndexedStringsStorage<IndType>::table_t& _table;
public:
#ifdef USE_HASHSET
    StringsEqualComparator<IndType>(const typename IndexedStringsStorage<IndType>::table_t& table): _table(table) {}
#else
    StringsLessComparator<IndType>(const typename IndexedStringsStorage<IndType>::table_t& table): _table(table) {}
#endif

    /** Comparison of two pointers to C-strings.
     * @param lhs Pointer to 1st C-string.
     * @param rhs Pointer to 2nd C-string.
     * @return True, if 1st argument is equal/less than 2nd argument.
     */
    inline bool operator()(IndType lhs, IndType rhs) const {
#ifdef USE_HASHSET
        return strcmp(_table[lhs], _table[rhs]) == 0;
#else
        return strcmp(_table[lhs], _table[rhs]) < 0;
#endif
    }
};

#ifdef USE_HASHSET
/** Functor... TODO.
 */
template<class IndType>
class Hasher: public std::unary_function<IndType, size_t> {

    __gnu_cxx::hash<const char*> _hash;

    /** @var conversion table: index -> string (necessary for indices comparison) */
    const typename IndexedStringsStorage<IndType>::table_t& _table;

public:
    /** */
    Hasher<IndType>(const typename IndexedStringsStorage<IndType>::table_t& table): _hash(), _table(table) {}

    /** Hashing function.
     * @param index
     * @return Counted hash.
     */
    inline size_t operator()(const IndType index) const {
        return _hash(_table[index]);
    }
};
#endif

template <class IndType>
#ifdef USE_HASHSET
IndexedStringsStorage<IndType>::IndexedStringsStorage(void): _storage(sizeof(char)), _table(), _index(100, hasher_t(_table), equality_comparator_t(_table)) {}
#else
IndexedStringsStorage<IndType>::IndexedStringsStorage(void): _storage(sizeof(char)), _table(), _index(less_comparator_t(_table)) {}
#endif

template <class IndType>
IndType IndexedStringsStorage<IndType>::put(const char* str) {

    if ( this->is_full() ) {
        // What a pity, not a single index left to spend.
        throw std::bad_alloc();
    }

    // To use the index for lookup we first have to store passed string
    // in conversion table (cause during lookup we compare the strings indirectly
    // by using their indices).
    // Note: thread unsafe! TODO: Redesing.
    IndType index = static_cast<IndType>(_table.size());
    _table.push_back(str);

#ifdef USE_HASHSET
    //
    typename index_t::iterator iIndex = _index.find(index);
#else
    // A lower_bound() search enables us to use "found" iterator as a hint for
    // eventual insertion.
    typename index_t::iterator iIndex = _index.lower_bound(index);
#endif

    if ( (iIndex != _index.end())
#ifndef USE_HASHSET
        // In case of lower_bound() search we have to also compare found item
        // with passed string.
        && (strcmp(_table[*iIndex], str) == 0)
#endif
        ) {
        // String is already present in storage!
        // Pop back temporary stored pointer...
        _table.pop_back();
        // ...and return numeric index to already stored copy of `str`.
        return static_cast<IndType>(*iIndex);
    }

    // String not found within storage.

    // Allocate memory required for string storage...
    char* mem = static_cast<char*>(_storage.ordered_malloc(strlen(str) + 1));
    // ...and fill it with copy of passed string.
    strcpy(mem, str);

    // Overwrite temporary stored pointer to `str` with pointer to freshly
    // saved copy.
    _table[index] = mem;

#ifdef USE_HASHSET
    // Insert the index into lookup table.
    _index.insert(index);
#else
    // Insert the index into lookup table (use previously retrieved iterator
    // as a hint).
    _index.insert(iIndex, index);
#endif

    // Finally.
    return index;
}

#endif
