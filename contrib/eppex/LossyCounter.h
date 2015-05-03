/**
 * LossyCounter - implementation of Lossy Counting algorithm as described in:
 * Approximate Frequency Counts over Data Streams, G.S.Manku & R.Motwani, (2002)
 *
 * (C) Ceslav Przywara, UFAL MFF UK, 2011
 *
 * Implementation note: define USE_UNORDERED_MAP to use std::tr1::unordered_map
 * instead std::map for storage of lossy counted items.
 *
 * $Id$
 */

#ifndef LOSSYCOUNTER_H
#define	LOSSYCOUNTER_H

#include <cstddef>
#include <cmath>
#ifdef USE_UNORDERED_MAP
#include <tr1/unordered_map>
#else
#include <map>
#endif
#include <iterator>


// Iterators:
template<class T>
class LossyCounterIterator;

template<class T>
class LossyCounterErasingIterator;


////////////////////////////////////////////////////////////////////////////////
/////////////////////////// Lossy Counter Interface ////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * Implementation of Lossy Counting algorithm as described in:
 * Approximate Frequency Counts over Data Streams, G.S.Manku & R.Motwani, (2002)
 */
template<class T>
class LossyCounter {

public:

    // Error parameter type definition.
    typedef double error_t;

    // Support parameter type definition.
    typedef double support_t;

    // Counters type definition.
    typedef size_t counter_t;

    // Frequency counter type definition (f).
    typedef counter_t frequency_t;

    // Maximum error counter type definition (Δ).
    typedef counter_t maximum_error_t;

    // Pair: frequency (f) and possible maximum error (Δ).
    typedef std::pair<frequency_t, maximum_error_t> item_counts_t;
#ifdef USE_UNORDERED_MAP
    typedef std::tr1::unordered_map<T, item_counts_t, typename T::Hash> storage_t;
#else
    // Mapping: counted item and its frequency and max. error counts.
    typedef std::map<T, item_counts_t> storage_t;
#endif
    // We provide own version of const iterator.
    typedef LossyCounterIterator<T> const_iterator;

    // Special type of iterator: leaves no item behind!
    typedef LossyCounterErasingIterator<T> erasing_iterator;

    /** @var Error parameter value (ε) */
    const error_t error;

    /** @var Supprort parameter value (s) */
    const support_t support;

    /** @var Width of single bucket (w) */
    const counter_t bucketWidth; // ceil(1/error)

private:

    /** @var Current epoch bucket ID (b-current) */
    counter_t _bucketId;

    /** @var Count of items read in so far (N) */
    counter_t _count;

    /** @var Items storage (Ɗ) */
    storage_t _storage;

public:

    /**
     * Set error to 0 to disable lossy-pruning.
     * @param _error Value from interval [0.0, 1.0).
     * @param _support Value from interval [0.0, 1.0).
     */
    LossyCounter<T>(error_t _error, support_t _support):
        error(_error), support(_support), bucketWidth(_error > 0.0 ? ceil(1/_error) : 0), _bucketId(1), _count(0), _storage() {}

    /**
     * @param item Item to be added to storage.
     */
    void add(const T& item);

    /**
     * @return Constant iterator pointing to the beginning of storage.
     */
    const_iterator begin(void) const { return LossyCounterIterator<T>(threshold(), _storage.begin(), _storage.end()); }

    /**
     * @return Constant iterator pointing to the end of storage.
     */
    const_iterator end(void) const { return LossyCounterIterator<T>(_storage.end()); }

    /**
     * @return Erasing iterator pointing to the beginning of storage.
     */
    erasing_iterator beginErase(void) { return LossyCounterErasingIterator<T>(threshold(), _storage); }

    /**
     * @return Erasing iterator pointing to the end of storage.
     */
    erasing_iterator endErase(void) { return LossyCounterErasingIterator<T>(_storage); }

    /**
     * @return Current bucket ID.
     */
    counter_t bucketId(void) const { return _bucketId; }

    /**
     * @return Number of items added to storage so far (N).
     */
    counter_t count(void) const { return _count; }

    /**
     * @return Number of items currently in storage.
     */
    size_t size(void) const { return _storage.size(); }

    /**
     * @param positive Return sN value instead of (s-ε)N?
     * @return Threshold (either positive or negative) value.
     */
    double threshold(bool positive = false) const { return positive ? support * _count : (support - error) * _count; }

    /**
     * @return The maximum value of which estimated frequencies are less than the true frequencies.
     */
    double maxError(void) const { return error * _count; }

    /**
     * @return True, if it's prunning time right now!
     */
    bool aboutToPrune(void) const { return (bucketWidth != 0) && ((_count % bucketWidth) == 0); }

private:

    /**
     * Prunes counts table.
     */
    void prune(void);

};


////////////////////////////////////////////////////////////////////////////////
/////////////////////// Lossy Counter Iterator Interface ///////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * Lossy counter iterator is designed to iterate over items passing the current
 * threshold.
 */
template<class T>
class LossyCounterIterator: public std::iterator<std::forward_iterator_tag, typename LossyCounter<T>::storage_t::value_type> {
public:

    typedef LossyCounterIterator<T> self_type;

    typedef typename LossyCounter<T>::storage_t::const_iterator const_iterator;

protected:

    /** @var Minimum frequency threshold */
    const double _threshold;

    /** @var Current position of iterator (based on underlying container) */
    const_iterator _current;

    /** @var End position in underlying container */
    const const_iterator _end;

public:

    // Constructors.

    LossyCounterIterator<T>(const_iterator end):
        _threshold(0), _current(_end), _end(end) {}

    LossyCounterIterator<T>(double threshold, const_iterator begin, const_iterator end):
        _threshold(threshold), _current(begin), _end(end) {
        // Forward the iterator to the first valid item (with frequency above threshold)!
        this->forward(true);
    }

    // Operators.

    /**
     * Only items passing the threshold are included in iteration.
     */
    LossyCounterIterator<T> operator++(void); // ++this

    /**
     * Only items passing the threshold are included in iteration.
     */
    LossyCounterIterator<T> operator++(int junk); // this++

    bool operator==(const self_type& rhs) const { return _current == rhs._current; }

    bool operator!=(const self_type& rhs) const { return _current != rhs._current; }

    // Interface.

    /**
     * @return Current item.
     */
    const T& item(void) const { return _current->first; }

    /**
     * @return Current item's frequency.
     */
    typename LossyCounter<T>::frequency_t frequency(void) const { return _current->second.first; }

    /**
     * @return Current item's maximum error.
     */
    typename LossyCounter<T>::error_t max_error(void) const { return _current->second.second; }

protected:

    /**
     * @param init Check also the item that iterator _current points to? Useful when initializing.
     */
    void forward(bool init);

};

/**
 * Lossy counter iterator erasing all items passed by.
 */
template<class T>
class LossyCounterErasingIterator: public LossyCounterIterator<T> {
public:

    typedef typename LossyCounter<T>::storage_t storage_t;

private:

    storage_t& _storage;

public:

    // Constructors - have to be aware of the "mother" container.

    LossyCounterErasingIterator<T>(storage_t& storage): LossyCounterIterator<T>(storage.end()), _storage(storage) {}

    LossyCounterErasingIterator<T>(double threshold, storage_t& storage): LossyCounterIterator<T>(threshold, storage.begin(), storage.end()), _storage(storage) {}

protected:

    /**
     * @param init Check also the item that iterator _current points to? Useful when initializing.
     */
    void forward(bool init);

};


////////////////////////////////////////////////////////////////////////////////
//////////////////////// Lossy Counter Implementation //////////////////////////
////////////////////////////////////////////////////////////////////////////////

template<class T>
void LossyCounter<T>::add(const T& item) {

    typename storage_t::iterator iter = _storage.find(item);

    if ( iter == _storage.end() ) {
        // Insert new item with appropriate frequency and maximum-possible-error.
        _storage.insert(std::make_pair(item, item_counts_t(1, _bucketId - 1)));
    }
    else {
        // Update frequency of existing.
        iter->second.first += 1;
    }

    // Finally increment the counter and check if the table shall be pruned.
    ++_count;
    if ( this->aboutToPrune() ) {
        this->prune();
        ++_bucketId;
    }
}


template<class T>
void LossyCounter<T>::prune(void) {

    for ( typename storage_t::iterator iter = _storage.begin(); iter != _storage.end(); /* Incrementation step missing intentionally! */ ) {
        // Prune, if: maximum possible error + frequency <= ID of current bucket
        if ( (iter->second.first + iter->second.second) <= _bucketId ) {
            _storage.erase(iter++); // Post increment!
        }
        else {
            ++iter;
        }
    }

}


////////////////////////////////////////////////////////////////////////////////
//////////////////// Lossy Counter Iterator Implementation /////////////////////
////////////////////////////////////////////////////////////////////////////////

template<class T>
LossyCounterIterator<T> LossyCounterIterator<T>::operator++(void) {
    this->forward();
    return *this;
}


template<class T>
LossyCounterIterator<T> LossyCounterIterator<T>::operator++(int junk) {
    self_type iter = *this;
    this->forward();
    return iter;
}

template<class T>
void LossyCounterIterator<T>::forward(bool init = false) {
    if ( _current == _end ) {
        return; // Nowhere to go, we're at the end already.
    }
    if ( init && (this->frequency() >= _threshold) ) {
        // Ok, we're initing and we're already at the item passing the threshold.
        return;
    }
    // Keep going until we reach the end or the next item with frequency NOT below the threshold.
    while ( (++_current != _end) && (this->frequency() < _threshold) );
}


////////////////////////////////////////////////////////////////////////////////
//////////////// Lossy Counter Erasing Iterator Implementation /////////////////
////////////////////////////////////////////////////////////////////////////////

template<class T>
void LossyCounterErasingIterator<T>::forward(bool init = false) {
    if (this->_current == this->_end ) {
        return; // Nowhere to go, we're at the end already.
    }
    if ( init && (this->frequency() >= this->_threshold) ) {
        // Ok, we're initing and we're already at the item passing the threshold.
        return;
    }
    // This is where erasing forward() and std forward() differ:
    typename LossyCounterIterator<T>::const_iterator previous = this->_current;
    // Keep going...
    while ( ++this->_current != this->_end ) {
        if ( this->frequency() < this->_threshold ) {
            // Get rid of previous, cause we'll go on.
            _storage.erase(previous);
            previous = this->_current;
        } else {
            break;
        }
    }
    _storage.erase(previous); // !
}

#endif	/* LOSSYCOUNTER_H */
